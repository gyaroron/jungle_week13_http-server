#include "conn.h"
#include "config.h"
#include "log.h"
#include "net.h"
#include "route.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static volatile sig_atomic_t g_stop = 0;
static const struct config *g_worker_config = NULL;
static pthread_mutex_t g_active_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_active_changed = PTHREAD_COND_INITIALIZER;
static size_t g_active_threads = 0;

struct connection_job {
    int fd;
};

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

// 연결별 스레드는 heap에 받은 연결 번호를 꺼낸 뒤, 기존 연결 처리기를 실행합니다.
static void *serve_connection_thread(void *argument) {
    struct connection_job *job = argument;
    int fd = job->fd;
    free(job);

    serve_connection(fd);

    int result = pthread_mutex_lock(&g_active_mutex);
    if (result != 0) {
        server_log_errno("pthread_mutex_lock active", result);
        _exit(1);
    }
    g_active_threads--;
    result = pthread_cond_broadcast(&g_active_changed);
    int unlock_result = pthread_mutex_unlock(&g_active_mutex);
    if (result != 0 || unlock_result != 0) {
        server_log_errno("pthread connection completion",
                         result != 0 ? result : unlock_result);
        _exit(1);
    }
    return NULL;
}

// 부분적으로 읽은 라우팅 규칙도 초기화된 mutex만 정리합니다.
static void cleanup_routes(struct route *routes, int route_count) {
    route_destroy(routes, route_count);
    free(routes);
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

    // 연결별 스레드가 끝날 때까지 설정과 라우트를 읽기 전용으로 공유합니다.
    g_worker_config = &cfg;
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

        // 스레드가 연결 번호를 받기 전까지 보관할 heap 공간을 만듭니다.
        struct connection_job *job = malloc(sizeof(*job));
        if (job == NULL) {
            server_log_errno("allocate connection job", errno);
            close(conn_fd);
            continue;
        }
        job->fd = conn_fd;

        pthread_t thread;
        int thread_error = pthread_mutex_lock(&g_active_mutex);
        if (thread_error != 0) {
            server_log_errno("pthread_mutex_lock active", thread_error);
            free(job);
            close(conn_fd);
            continue;
        }
        g_active_threads++;
        thread_error = pthread_create(&thread, NULL,
                                      serve_connection_thread, job);
        if (thread_error != 0) {
            g_active_threads--;
            pthread_mutex_unlock(&g_active_mutex);
            server_log_errno("pthread_create", thread_error);
            free(job);
            close(conn_fd);
            continue;
        }
        int unlock_error = pthread_mutex_unlock(&g_active_mutex);
        if (unlock_error != 0) {
            server_log_errno("pthread_mutex_unlock active", unlock_error);
            _exit(1);
        }

        // 분리한 스레드는 연결 하나를 끝낸 뒤 join 없이 종료합니다.
        thread_error = pthread_detach(thread);
        if (thread_error != 0) {
            server_log_errno("pthread_detach", thread_error);
            thread_error = pthread_join(thread, NULL);
            if (thread_error != 0) {
                server_log_errno("pthread_join fallback", thread_error);
                _exit(1);
            }
        }
    }

    close(listen_fd);
    // detached 스레드가 설정과 라우트를 다 쓸 때까지 main이 정리하지 않습니다.
    int wait_error = pthread_mutex_lock(&g_active_mutex);
    if (wait_error != 0) {
        server_log_errno("pthread_mutex_lock shutdown", wait_error);
        _exit(1);
    }
    while (g_active_threads > 0) {
        wait_error = pthread_cond_wait(&g_active_changed, &g_active_mutex);
        if (wait_error != 0) {
            pthread_mutex_unlock(&g_active_mutex);
            server_log_errno("pthread_cond_wait shutdown", wait_error);
            _exit(1);
        }
    }
    wait_error = pthread_mutex_unlock(&g_active_mutex);
    if (wait_error != 0) {
        server_log_errno("pthread_mutex_unlock shutdown", wait_error);
        _exit(1);
    }
    pthread_cond_destroy(&g_active_changed);
    pthread_mutex_destroy(&g_active_mutex);
    server_log("server stopped\n");
    cleanup_routes(routes, cfg.route_count);
    return 0;
}
