#include "conn.h"
#include "cgi.h"
#include "cache.h"
#include "http.h"
#include "log.h"
#include "net.h"
#include "proxy.h"
#include "response.h"
#include "static.h"

#include <errno.h>
#include <stdio.h>
#include <limits.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

// 이 값들은 TCP 상태가 아니라 서버 프로그램이 관리하는 처리 단계입니다.
enum conn_state {
    CONN_READING,
    CONN_PARSING,
    CONN_ROUTING,
    CONN_RESPONDING,
    CONN_KEEPALIVE_CHECK,
    CONN_CLOSING,
    CONN_CLOSED
};

// 연결 하나를 처리하는 동안 필요한 번호, 현재 단계, 요청 저장 공간입니다.
struct conn {
    int fd;
    enum conn_state state;
    char rbuf[8192];
    size_t rlen;
    size_t request_len;
    int keep_alive;
    int response_sent;
    char *body;
    size_t body_len;
    // 요청 하나(헤더와 본문)를 다 받아야 하는 단조 시계 기준 마감 시각입니다.
    // recv() 한 번마다 늘어나는 소켓 제한 시간과 달리, 조금씩 보내는 연결도 이 시각에 끊깁니다.
    long long deadline_ms;
    int timeout_ms;
    // event loop 모드에서 worker가 바뀌어도 처리를 이어 가도록 연결별 정보를 함께 둡니다.
    const struct config *cfg;
    char client_ip[64];
};

static long long monotonic_ms(void) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) < 0) return 0;
    return (long long)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

// 다음 요청의 읽기 마감 시각을 지금부터 제한 시간 뒤로 정합니다.
static void conn_start_request_clock(struct conn *c) {
    c->deadline_ms = monotonic_ms() + c->timeout_ms;
}

// 읽을 데이터가 오면 1, 마감 시각이 지나면 0, poll 오류면 -1을 돌려줍니다.
static int conn_wait_readable(const struct conn *c) {
    for (;;) {
        long long left = c->deadline_ms - monotonic_ms();
        if (left <= 0) return 0;
        int ready = net_wait(c->fd, POLLIN, left > INT_MAX ? INT_MAX : (int)left);
        if (ready < 0 && errno == EINTR) continue;
        return ready;
    }
}

// 상태 출력에 사용할 단계 이름을 돌려줍니다.
static const char *conn_state_name(enum conn_state state) {
    switch (state) {
        case CONN_READING: return "READING";
        case CONN_PARSING: return "PARSING";
        case CONN_ROUTING: return "ROUTING";
        case CONN_RESPONDING: return "RESPONDING";
        case CONN_KEEPALIVE_CHECK: return "KEEPALIVE_CHECK";
        case CONN_CLOSING: return "CLOSING";
        case CONN_CLOSED: return "CLOSED";
    }
    return "UNKNOWN";
}

// 단계가 바뀌면 서버 터미널의 오류 출력(stderr)에 전환을 기록합니다.
static void conn_set_state(struct conn *c, enum conn_state next) {
    server_log("fd %d: %s -> %s\n", c->fd,
               conn_state_name(c->state), conn_state_name(next));
    c->state = next;
}

// 요청 버퍼 안에서 CRLF 두 쌍을 길이 제한 안에 찾아냅니다.
static int conn_has_header_end(const char *buffer, size_t length) {
    if (length < 4) return 0;
    for (size_t i = 0; i <= length - 4; i++) {
        if (memcmp(buffer + i, "\r\n\r\n", 4) == 0) return 1;
    }
    return 0;
}

// 파싱 실패 결과에 맞는 짧은 HTTP 오류 응답을 고릅니다.
static int conn_error_status(enum http_result result) {
    switch (result) {
        case HTTP_BAD_REQUEST: return 400;
        case HTTP_URI_TOO_LONG: return 414;
        case HTTP_HEADER_TOO_LARGE: return 431;
        case HTTP_NOT_IMPLEMENTED: return 501;
        case HTTP_VERSION_NOT_SUPPORTED: return 505;
        case HTTP_PAYLOAD_TOO_LARGE: return 413;
        case HTTP_OK:
        case HTTP_INCOMPLETE:
            return 500;
    }
    return 500;
}

// 연결 하나의 단계를 CLOSED 또는 (event loop 모드에서) 다음 요청 대기까지 진행합니다.
static enum conn_run_result conn_run(struct conn *c, int event_mode) {
    // 현재 단계가 닫힘에 도달할 때까지 이 연결의 처리 단계를 진행합니다.
    while (c->state != CONN_CLOSED) {
        switch (c->state) {
            case CONN_READING: {
                // 앞 요청을 처리하고도 다음 요청 바이트가 남았는지 먼저 확인합니다.
                if (conn_has_header_end(c->rbuf, c->rlen)) {
                    server_log("fd %d: request data available (%zu bytes)\n",
                               c->fd, c->rlen);
                    conn_set_state(c, CONN_PARSING);
                    break;
                }

                // event loop 모드에서는 여기서 기다리지 않습니다. 연결을 loop에 돌려주면
                // loop가 kqueue/epoll로 다음 바이트를 기다리므로 worker가 묶이지 않습니다.
                if (event_mode) {
                    return CONN_RUN_PARKED;
                }

                // 아직 비어 있는 버퍼 영역의 크기를 구해 넘칠 공간이 없도록 합니다.
                size_t available = sizeof(c->rbuf) - c->rlen - 1;
                if (available == 0) {
                    server_log("fd %d: request headers exceed buffer capacity\n",
                               c->fd);
                    conn_set_state(c, CONN_CLOSING);
                    break;
                }

                // 마감 시각 안에 바이트가 오면 rbuf의 기존 데이터 뒤에 이어 받습니다.
                // 마감이 지나면 소켓 제한 시간이 끝난 경우와 같은 경로(408 후 종료)로 보냅니다.
                ssize_t n;
                int ready = conn_wait_readable(c);
                if (ready <= 0) {
                    n = -1;
                    if (ready == 0) errno = EAGAIN;
                } else {
                    n = recv(c->fd, c->rbuf + c->rlen, available, 0);
                }
                if (n < 0) {
                    if (errno == EINTR) break;
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        if (c->rlen > 0) {
                            resp_send_error(c->fd, 408);
                        }
                        server_log("fd %d: request read timed out\n", c->fd);
                    } else {
                        server_log_errno("recv", errno);
                    }
                    conn_set_state(c, CONN_CLOSING);
                } else if (n == 0) {
                    server_log("fd %d: peer closed\n", c->fd);
                    conn_set_state(c, CONN_CLOSING);
                } else {
                    // 이번에 읽은 바이트 수를 누적하고 문자열 끝을 표시합니다.
                    c->rlen += (size_t)n;
                    c->rbuf[c->rlen] = '\0';

                    // HTTP 헤더 끝의 빈 줄이 도착했을 때만 요청 읽기를 마칩니다.
                    if (conn_has_header_end(c->rbuf, c->rlen)) {
                        server_log("fd %d: request data available (%zu bytes)\n",
                                   c->fd, c->rlen);
                        conn_set_state(c, CONN_PARSING);
                    } else if (c->rlen == sizeof(c->rbuf) - 1) {
                        // 용량을 다 썼는데 끝 표시가 없으면 버퍼를 더 쓰지 않고 연결을 닫습니다.
                        server_log("fd %d: request headers exceed buffer capacity\n",
                                   c->fd);
                        conn_set_state(c, CONN_CLOSING);
                    }
                }
                break;
            }

            case CONN_PARSING: {
                // 원문 요청 줄을 검증하고 method, target, version 필드로 나눕니다.
                struct http_request req;
                enum http_result result = http_parse(c->rbuf, c->rlen, &req);
                server_log("fd %d: parse result %s\n", c->fd,
                           http_result_name(result));
                if (result == HTTP_OK) {
                    c->request_len = req.header_len + (size_t)req.content_length;
                    const char *connection = http_header_get(&req, "Connection");
                    c->keep_alive = strcmp(req.version, "HTTP/1.1") == 0;
                    if (connection != NULL) {
                        if (strcasecmp(connection, "close") == 0) {
                            c->keep_alive = 0;
                        } else if (strcasecmp(connection, "keep-alive") == 0) {
                            c->keep_alive = 1;
                        }
                    }
                    req.keep_alive = c->keep_alive;
                    server_log("fd %d: request %s %s %s; headers=%d bytes=%zu\n",
                               c->fd, req.method, req.target, req.version,
                               req.nheaders, req.header_len);
                    if (req.content_length > 0) {
                        size_t expected = (size_t)req.content_length;
                        c->body = malloc(expected + 1);
                        if (c->body == NULL) {
                            server_log_errno("malloc request body", errno);
                            conn_set_state(c, CONN_CLOSING);
                            break;
                        }

                        // 헤더와 함께 이미 받은 본문 바이트를 새 저장 공간으로 옮깁니다.
                        size_t received = c->rlen - req.header_len;
                        if (received > expected) {
                            received = expected;
                        }
                        memcpy(c->body, c->rbuf + req.header_len, received);

                        // Content-Length가 가리키는 나머지 바이트만 더 받습니다.
                        while (received < expected) {
                            // 본문도 같은 요청 마감 시각 안에 모두 받아야 합니다.
                            ssize_t n;
                            int ready = conn_wait_readable(c);
                            if (ready <= 0) {
                                n = -1;
                                if (ready == 0) errno = EAGAIN;
                            } else {
                                n = recv(c->fd, c->body + received,
                                         expected - received, 0);
                            }
                            if (n < 0) {
                                if (errno == EINTR) continue;
                                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                    resp_send_error(c->fd, 408);
                                } else {
                                    server_log_errno("recv body", errno);
                                }
                                free(c->body);
                                c->body = NULL;
                                conn_set_state(c, CONN_CLOSING);
                                break;
                            }
                            if (n == 0) {
                                server_log("fd %d: peer closed during request body\n",
                                           c->fd);
                                free(c->body);
                                c->body = NULL;
                                conn_set_state(c, CONN_CLOSING);
                                break;
                            }
                            received += (size_t)n;
                        }
                        if (c->state == CONN_CLOSING) {
                            break;
                        }
                        c->body[received] = '\0';
                        c->body_len = received;
                        server_log("fd %d: request body bytes=%zu\n",
                                   c->fd, received);
                    }
                    conn_set_state(c, CONN_ROUTING);
                } else {
                    if (resp_send_error(c->fd, conn_error_status(result)) < 0) {
                        server_log_errno("send error response", errno);
                    }
                    conn_set_state(c, CONN_CLOSING);
                }
                break;
            }

            case CONN_ROUTING: {
                // 프록시 규칙이 먼저 맞는지 보고, 아니면 정적 파일이나 CGI로 보냅니다.
                struct http_request req;
                enum http_result parse_result = http_parse(c->rbuf, c->rlen, &req);
                if (parse_result != HTTP_OK) {
                    conn_set_state(c, CONN_CLOSING);
                    break;
                }
                req.keep_alive = c->keep_alive;
                int serve_result = 0;
                int proxy_handled = 0;
                int invalidates_cache = strcmp(req.method, "GET") != 0;
                if (invalidates_cache) {
                    // 기존 응답을 지우고, 이 요청이 끝날 때까지 GET이 캐시를 건너뛰게 합니다.
                    // 요청 처리 동안 잠금을 잡지 않으므로 다른 연결은 기다리지 않습니다.
                    cache_write_begin();
                }
                struct cache_capture capture;
                struct cache_capture *capture_ptr = NULL;
                char cache_key[CACHE_KEY_MAX];
                if (strncmp(req.target, "/cgi-bin/", 9) != 0 &&
                    cache_key_for_request(&req, cache_key, sizeof(cache_key))) {
                    int cache_result = cache_lookup_send(c->fd, cache_key,
                                                         c->keep_alive);
                    if (cache_result < 0) {
                        conn_set_state(c, CONN_CLOSING);
                        break;
                    }
                    if (cache_result > 0) {
                        // HIT 응답은 저장된 Content-Length로 끝나므로 연결을 유지할 수 있습니다.
                        c->response_sent = 1;
                        conn_set_state(c, CONN_RESPONDING);
                        break;
                    }
                    if (cache_capture_begin(&capture, c->fd, cache_key) == 0) {
                        capture_ptr = &capture;
                    }
                }
                if (c->cfg->forward_proxy &&
                    (strncmp(req.target, "http://", 7) == 0 ||
                     strncmp(req.target, "https://", 8) == 0)) {
                    serve_result = proxy_forward_cached(c->fd, &req, c->body,
                                                        c->body_len,
                                                        c->cfg->timeout_sec,
                                                        c->client_ip, capture_ptr);
                    proxy_handled = 1;
                } else {
                    int reverse_result = proxy_reverse_cached(c->fd, &req, c->body,
                                                              c->body_len, c->cfg,
                                                              c->client_ip,
                                                              capture_ptr);
                    if (reverse_result == PROXY_NO_ROUTE) {
                        if (strncmp(req.target, "/cgi-bin/", 9) == 0) {
                            serve_result = cgi_serve(c->fd, c->cfg, &req,
                                                     c->body, c->body_len);
                        } else {
                            serve_result = static_serve_cached(c->fd, c->cfg, &req,
                                                               capture_ptr);
                        }
                    } else {
                        serve_result = reverse_result == PROXY_FAILED ? -1 : 0;
                        proxy_handled = 1;
                    }
                }
                if (invalidates_cache) {
                    // 변경 응답을 마친 뒤 다시 비워 변경 중 생성된 항목도 제거합니다.
                    cache_write_end();
                }
                if (serve_result < 0) {
                    cache_capture_abort(capture_ptr);
                    if (errno != EPIPE && errno != ECONNRESET) {
                        server_log_errno("static_serve", errno);
                    }
                    conn_set_state(c, CONN_CLOSING);
                    break;
                }
                if (capture_ptr != NULL && !capture_ptr->keep_alive)
                    c->keep_alive = 0;
                if (capture_ptr != NULL && cache_capture_finish(capture_ptr) < 0) {
                    conn_set_state(c, CONN_CLOSING);
                    break;
                }
                c->response_sent = 1;
                // 오류 응답과 프록시 중계(업스트림 EOF까지 전달)는 연결을 닫습니다.
                // 캐시 MISS 정적 응답은 cache_capture가 keep-alive 헤더를 맞춰 보냅니다.
                if (serve_result > 0 || proxy_handled)
                    c->keep_alive = 0;
                conn_set_state(c, CONN_RESPONDING);
                break;
            }

            case CONN_RESPONDING: {
                if (c->response_sent) {
                    conn_set_state(c, CONN_KEEPALIVE_CHECK);
                    break;
                }
                static const char body[] = "hello\n";
                if (resp_send_simple(c->fd, 200, "text/plain; charset=utf-8",
                                     body, sizeof(body) - 1, c->keep_alive) < 0) {
                    if (errno != EPIPE && errno != ECONNRESET) {
                        server_log_errno("send", errno);
                    }
                    conn_set_state(c, CONN_CLOSING);
                    break;
                }
                conn_set_state(c, CONN_KEEPALIVE_CHECK);
                break;
            }

            case CONN_KEEPALIVE_CHECK:
                if (!c->keep_alive) {
                    conn_set_state(c, CONN_CLOSING);
                    break;
                }

                // 방금 처리한 요청만 버리고, 다음 요청 바이트는 버퍼 앞으로 당깁니다.
                if (c->request_len > c->rlen) {
                    c->request_len = c->rlen;
                }
                c->rlen -= c->request_len;
                memmove(c->rbuf, c->rbuf + c->request_len, c->rlen);
                c->rbuf[c->rlen] = '\0';
                free(c->body);
                c->body = NULL;
                c->body_len = 0;
                c->response_sent = 0;
                conn_start_request_clock(c);
                conn_set_state(c, CONN_READING);
                break;

            case CONN_CLOSING:
                // 이 연결의 처리를 마치고 서버 프로세스의 fd를 커널에 반납합니다.
                free(c->body);
                c->body = NULL;
                // event loop 모드에서는 호출자가 연결 목록에서 뺀 뒤 conn_destroy()로 닫습니다.
                if (!event_mode) {
                    close(c->fd);
                    c->fd = -1;
                }
                conn_set_state(c, CONN_CLOSED);
                break;

            case CONN_CLOSED:
                break;
        }
    }
    return CONN_RUN_CLOSED;
}

void conn_handle(int fd, const char *client_ip, const struct config *cfg) {
    struct conn c = {
        .fd = fd,
        .state = CONN_READING,
        .rlen = 0,
        .timeout_ms = cfg->timeout_sec * 1000,
        .cfg = cfg
    };
    snprintf(c.client_ip, sizeof(c.client_ip), "%s", client_ip != NULL ? client_ip : "unknown");
    conn_start_request_clock(&c);
    (void)conn_run(&c, 0);
}

long long conn_now_ms(void) {
    return monotonic_ms();
}

struct conn *conn_create(int fd, const char *client_ip, const struct config *cfg) {
    struct conn *c = calloc(1, sizeof(*c));
    if (c == NULL) return NULL;
    c->fd = fd;
    c->state = CONN_READING;
    c->timeout_ms = cfg->timeout_sec * 1000;
    c->cfg = cfg;
    snprintf(c->client_ip, sizeof(c->client_ip), "%s", client_ip != NULL ? client_ip : "unknown");
    conn_start_request_clock(c);
    return c;
}

int conn_get_fd(const struct conn *c) {
    return c->fd;
}

long long conn_deadline_ms(const struct conn *c) {
    return c->deadline_ms;
}

int conn_has_partial_request(const struct conn *c) {
    return c->rlen > 0;
}

enum conn_read_result conn_read_available(struct conn *c) {
    size_t available = sizeof(c->rbuf) - c->rlen - 1;
    if (available == 0) {
        server_log("fd %d: request headers exceed buffer capacity\n", c->fd);
        return CONN_READ_CLOSE;
    }
    // 소켓은 worker의 차단 송수신을 위해 Blocking으로 두고, 이 호출만 MSG_DONTWAIT로 읽습니다.
    ssize_t n = recv(c->fd, c->rbuf + c->rlen, available, MSG_DONTWAIT);
    if (n < 0) {
        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) return CONN_READ_MORE;
        if (errno == ECONNRESET) {
            server_log("fd %d: peer reset\n", c->fd);
            return CONN_READ_CLOSE;
        }
        server_log_errno("recv", errno);
        return CONN_READ_CLOSE;
    }
    if (n == 0) {
        server_log("fd %d: peer closed\n", c->fd);
        return CONN_READ_CLOSE;
    }
    c->rlen += (size_t)n;
    c->rbuf[c->rlen] = '\0';
    if (conn_has_header_end(c->rbuf, c->rlen)) return CONN_READ_READY;
    if (c->rlen == sizeof(c->rbuf) - 1) {
        server_log("fd %d: request headers exceed buffer capacity\n", c->fd);
        return CONN_READ_CLOSE;
    }
    return CONN_READ_MORE;
}

enum conn_run_result conn_resume(struct conn *c) {
    return conn_run(c, 1);
}

void conn_destroy(struct conn *c) {
    if (c == NULL) return;
    free(c->body);
    if (c->fd >= 0) close(c->fd);
    free(c);
}
