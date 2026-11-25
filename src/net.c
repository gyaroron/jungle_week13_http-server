#include "net.h"
#include "log.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

int net_listen(const char *host, int port, int backlog) {
    // 커널에 IPv4와 TCP 바이트 스트림을 사용하는 소켓 생성을 요청합니다.
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        server_log_errno("socket", errno);
        return -1;
    }

    // 서버를 다시 실행할 때 주소를 재사용할 수 있도록 소켓 옵션을 켭니다.
    int on = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        server_log_errno("setsockopt", errno);
        close(listen_fd);
        return -1;
    }

    // bind()가 사용할 IPv4 주소와 포트를 담을 구조체를 준비합니다.
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);

    // host 문자열을 구조체가 사용할 IPv4 주소 값으로 바꿉니다.
    int address_result = inet_pton(AF_INET, host, &addr.sin_addr);
    if (address_result != 1) {
        if (address_result == 0) {
            server_log("invalid IPv4 address: %s\n", host);
        } else {
            server_log_errno("inet_pton", errno);
        }
        close(listen_fd);
        return -1;
    }

    // 커널에 listen_fd가 사용할 주소와 포트를 지정해 달라고 요청합니다.
    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        server_log_errno("bind", errno);
        close(listen_fd);
        return -1;
    }

    // 커널에 이 소켓을 연결 요청 대기 상태로 바꾸고 대기 큐 크기를 설정하도록 요청합니다.
    if (listen(listen_fd, backlog) < 0) {
        server_log_errno("listen", errno);
        close(listen_fd);
        return -1;
    }

    // 듣는 소켓 번호를 호출한 main()에 돌려줍니다.
    return listen_fd;
}

// 지정한 바이트를 모두 보낼 때까지 send()를 반복합니다.
int net_send_all(int fd, const void *buf, size_t len) {
    const char *bytes = buf;
    size_t sent = 0;

    while (sent < len) {
        ssize_t n = send(fd, bytes + sent, len - sent, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) {
            // 전송량이 늘지 않으면 더 진행할 수 없다는 오류로 돌려줍니다.
            errno = EPIPE;
            return -1;
        }
        sent += (size_t)n;
    }

    return 0;
}

int net_set_timeouts(int fd, int seconds) {
    struct timeval timeout = {.tv_sec = seconds, .tv_usec = 0};
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0 ||
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        return -1;
    }
    return 0;
}

int net_set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL);
    if (flags < 0) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int net_set_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL);
    if (flags < 0) return -1;
    return fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}

int net_wait(int fd, short events, int timeout_ms) {
    struct pollfd pfd = {.fd = fd, .events = events, .revents = 0};
    int result = poll(&pfd, 1, timeout_ms);
    if (result <= 0) return result;
    if (pfd.revents & (events | POLLHUP | POLLERR)) return 1;
    errno = EBADF;
    return -1;
}

// 한 업스트림 주소에 비차단 연결을 시도하고 결과를 제한 시간 안에 확인합니다.
int net_connect(const char *host, const char *port, int timeout_ms) {
    if (host == NULL || port == NULL || timeout_ms <= 0) {
        errno = EINVAL;
        return -1;
    }

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo *addresses = NULL;
    int lookup = getaddrinfo(host, port, &hints, &addresses);
    if (lookup != 0) {
        errno = EHOSTUNREACH;
        return -1;
    }

    int saved_error = ECONNREFUSED;
    int connected_fd = -1;
    for (struct addrinfo *address = addresses;
         address != NULL; address = address->ai_next) {
        int fd = socket(address->ai_family, address->ai_socktype,
                        address->ai_protocol);
        if (fd < 0) {
            saved_error = errno;
            continue;
        }
        if (net_set_nonblock(fd) < 0) {
            saved_error = errno;
            close(fd);
            continue;
        }

        int result = connect(fd, address->ai_addr, address->ai_addrlen);
        if (result < 0 && errno != EINPROGRESS && errno != EWOULDBLOCK &&
            errno != EALREADY && errno != EINTR) {
            saved_error = errno;
            close(fd);
            continue;
        }

        if (result < 0) {
            int ready;
            do {
                ready = net_wait(fd, POLLOUT, timeout_ms);
            } while (ready < 0 && errno == EINTR);
            if (ready == 0) {
                saved_error = ETIMEDOUT;
                close(fd);
                continue;
            }
            if (ready < 0) {
                saved_error = errno;
                close(fd);
                continue;
            }

            int socket_error = 0;
            socklen_t error_len = sizeof(socket_error);
            if (getsockopt(fd, SOL_SOCKET, SO_ERROR,
                           &socket_error, &error_len) < 0) {
                saved_error = errno;
                close(fd);
                continue;
            }
            if (socket_error != 0) {
                saved_error = socket_error;
                close(fd);
                continue;
            }
        }

        if (net_set_blocking(fd) < 0) {
            saved_error = errno;
            close(fd);
            continue;
        }

        struct timeval timeout = {
            .tv_sec = timeout_ms / 1000,
            .tv_usec = (timeout_ms % 1000) * 1000
        };
        if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,
                       &timeout, sizeof(timeout)) < 0 ||
            setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO,
                       &timeout, sizeof(timeout)) < 0) {
            saved_error = errno;
            close(fd);
            continue;
        }

#ifdef SO_NOSIGPIPE
        int no_sigpipe = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE,
                       &no_sigpipe, sizeof(no_sigpipe)) < 0) {
            saved_error = errno;
            close(fd);
            continue;
        }
#endif

        connected_fd = fd;
        break;
    }

    freeaddrinfo(addresses);
    if (connected_fd < 0) errno = saved_error;
    return connected_fd;
}
