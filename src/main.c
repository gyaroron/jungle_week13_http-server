#include "conn.h"
#include "config.h"
#include "log.h"
#include "net.h"
#include "pool.h"
#include "response.h"
#include "route.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static volatile sig_atomic_t g_stop = 0;
static const struct config *g_worker_config = NULL;

#define APP_QUEUE_CAP 128

// 일꾼이 받은 연결 번호를 기존 연결 처리기에 전달합니다.
static void serve_connection(int fd) {
    struct sockaddr_in peer;
    socklen_t peer_len = sizeof(peer);
    char client_ip[INET_ADDRSTRLEN] = "unknown";
    if (getpeername(fd, (struct sockaddr *)&peer, &peer_len) == 0) {
        if (inet_ntop(AF_INET, &peer.sin_addr, client_ip,
                      sizeof(client_ip)) == NULL) {
            client_ip[0] = '\0';
            memcpy(client_ip, "unknown", sizeof("unknown"));
        }
    }
    conn_handle(fd, client_ip, g_worker_config);
}

// 부분적으로 읽은 라우팅 규칙도 초기화된 mutex만 정리합니다.
static void cleanup_routes(struct route *routes, int route_count) {
    route_destroy(routes, route_count);
    free(routes);
}

// 종료 중이라 처리하지 않을 대기 연결에 503을 보내고 닫습니다.
static void reject_queued_connection(int fd) {
    (void)resp_send_error(fd, 503);
    close(fd);
}

static void request_stop(int signal_number) {
    (void)signal_number;
    g_stop = 1;
}

int main(int argc, char **argv) {
    struct config cfg;
    int config_result = config_parse(argc, argv, &cfg);
    if (config_result != 0) {
        return config_result > 0 ? 0 : 2;
    }

    struct route *routes = NULL;
    if (cfg.routes_path != NULL) {
        routes = calloc(ROUTE_MAX, sizeof(*routes));
        if (routes == NULL) {
            server_log_errno("allocate routes", errno);
            return 1;
        }
        int count = route_load(cfg.routes_path, routes, ROUTE_MAX);
        if (count < 0) {
            server_log_errno("load routes", errno);
            cleanup_routes(routes, 0);
            return 1;
        }
        cfg.routes = routes;
        cfg.route_count = count;
        server_log("loaded %d route rules from %s\n", count, cfg.routes_path);
    }

    // 종료 신호를 받으면 accept 반복을 끝내도록 상태만 표시합니다.
    struct sigaction stop_action = {0};
    stop_action.sa_handler = request_stop;
    sigemptyset(&stop_action.sa_mask);
    if (sigaction(SIGINT, &stop_action, NULL) < 0 ||
        sigaction(SIGTERM, &stop_action, NULL) < 0) {
        server_log_errno("sigaction stop", errno);
        cleanup_routes(routes, cfg.route_count);
        return 1;
    }

    // 끊긴 클라이언트에 send해도 SIGPIPE가 서버 프로세스를 끝내지 않게 합니다.
    struct sigaction ignore_pipe = {0};
    ignore_pipe.sa_handler = SIG_IGN;
    sigemptyset(&ignore_pipe.sa_mask);
    if (sigaction(SIGPIPE, &ignore_pipe, NULL) < 0) {
        server_log_errno("sigaction SIGPIPE", errno);
        cleanup_routes(routes, cfg.route_count);
        return 1;
    }

    // net_listen()에 서버 주소와 포트, 대기 큐 크기를 전달해 듣는 소켓을 준비시킵니다.
    int listen_fd = net_listen(cfg.host, cfg.port, cfg.backlog);
    if (listen_fd < 0) {
        // net_listen()이 실패 이유를 이미 출력했으므로 서버를 종료합니다.
        cleanup_routes(routes, cfg.route_count);
        return 1;
    }
    if (net_set_nonblock(listen_fd) < 0) {
        server_log_errno("fcntl listen nonblock", errno);
        close(listen_fd);
        cleanup_routes(routes, cfg.route_count);
        return 1;
    }

    // 서버가 연결을 기다리는 주소와 듣는 소켓 번호를 출력합니다.
    server_log("listening on %s:%d (fd %d)\n", cfg.host, cfg.port, listen_fd);

    // 설정은 일꾼이 모두 끝날 때까지 읽기 전용으로 공유합니다.
    g_worker_config = &cfg;
    struct pool *workers = pool_create(cfg.workers, APP_QUEUE_CAP,
                                      serve_connection);
    if (workers == NULL) {
        server_log_errno("pool_create", errno);
        close(listen_fd);
        cleanup_routes(routes, cfg.route_count);
        return 1;
    }

    // 서버를 계속 실행하며 연결 하나씩 차례대로 받습니다.
    while (!g_stop) {
        int ready = net_wait(listen_fd, POLLIN, 1000);
        if (ready == 0) continue;
        if (ready < 0) {
            if (errno == EINTR) continue;
            server_log_errno("poll", errno);
            continue;
        }

        // accept()가 접속한 클라이언트의 주소 정보를 기록할 공간입니다.
        struct sockaddr_in peer;
        socklen_t peer_len = sizeof(peer);

        // 커널에 listen_fd의 대기 연결 하나를 수락해 새 연결 소켓을 만들도록 요청합니다.
        int conn_fd = accept(listen_fd, (struct sockaddr *)&peer, &peer_len);
        if (conn_fd < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            // 한 연결을 받지 못해도 서버는 종료하지 않고 다음 연결 요청을 기다립니다.
            server_log_errno("accept", errno);
            continue;
        }

        // macOS는 듣는 소켓의 비차단 설정을 물려주므로 연결 소켓을 차단 모드로 되돌립니다.
        if (net_set_blocking(conn_fd) < 0) {
            server_log_errno("fcntl connection blocking", errno);
            close(conn_fd);
            continue;
        }
        if (net_set_timeouts(conn_fd, cfg.timeout_sec) < 0) {
            server_log_errno("setsockopt timeout", errno);
            close(conn_fd);
            continue;
        }

#ifdef SO_NOSIGPIPE
        // macOS 연결 소켓에 쓰기 중 SIGPIPE를 만들지 않도록 설정합니다.
        int no_sigpipe = 1;
        if (setsockopt(conn_fd, SOL_SOCKET, SO_NOSIGPIPE,
                       &no_sigpipe, sizeof(no_sigpipe)) < 0) {
            server_log_errno("setsockopt SO_NOSIGPIPE", errno);
            close(conn_fd);
            continue;
        }
#endif

        // 클라이언트 주소 구조체에 담긴 IPv4 주소를 사람이 읽는 문자열로 바꿉니다.
        char ip[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof(ip)) == NULL) {
            // 주소 변환에 실패하면 이 연결 소켓을 닫고 다음 연결을 기다립니다.
            server_log_errno("inet_ntop", errno);
            close(conn_fd);
            continue;
        }
        // 새 연결 소켓 번호와 접속한 클라이언트 주소를 출력합니다.
        server_log("accepted fd %d from %s:%d\n", conn_fd, ip,
                   ntohs(peer.sin_port));

        // 연결 번호를 원형 대기열에 넣고, 일꾼이 연결 처리를 맡게 합니다.
        if (pool_submit(workers, conn_fd) < 0) {
            if (errno == EAGAIN) {
                // 일꾼과 대기열이 모두 찼으면 짧은 503을 보내고 닫아 수락 반복을 지킵니다.
                server_log("fd %d: worker queue full, sending 503\n", conn_fd);
                (void)resp_send_error(conn_fd, 503);
            } else {
                server_log_errno("pool_submit", errno);
            }
            close(conn_fd);
        }
    }

    close(listen_fd);
    // 처리 중인 요청은 끝내되, 아직 시작하지 않은 대기 연결은 기다리지 않고 503으로 돌려보냅니다.
    int rejected = pool_reject_queued(workers, reject_queued_connection);
    if (rejected > 0) server_log("shutdown: rejected %d queued connections with 503\n", rejected);
    pool_shutdown(workers);
    server_log("server stopped\n");
    cleanup_routes(routes, cfg.route_count);
    // stderr가 막혀 있어도 종료가 멈추지 않도록 최대 1초만 기다립니다.
    server_log_flush(1000);
    return 0;
}
