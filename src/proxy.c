#include "proxy.h"

#include "cache.h"
#include "log.h"
#include "net.h"
#include "response.h"
#include "route.h"

#include <ctype.h>
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#define PROXY_HEADER_CAP 65536
#define PROXY_IO_CAP 16384

struct upstream {
    char host[256];
    char port[8];
    char host_header[272];
};

// 버퍼 범위를 넘지 않도록 형식 문자열을 뒤에 덧붙입니다.
static int append_format(char *buffer, size_t capacity, size_t *used,
                         const char *format, ...) {
    if (*used >= capacity) return -1;
    va_list args;
    va_start(args, format);
    int result = vsnprintf(buffer + *used, capacity - *used, format, args);
    va_end(args);
    if (result < 0 || (size_t)result >= capacity - *used) return -1;
    *used += (size_t)result;
    return 0;
}

// HTTP 헤더 이름에 허용된 토큰 문자만 있는지 검사합니다.
static int valid_header_name(const char *name) {
    if (name == NULL || *name == '\0') return 0;
    for (const unsigned char *p = (const unsigned char *)name; *p; p++) {
        if (isalnum(*p)) continue;
        if (strchr("!#$%&'*+-.^_`|~", *p) == NULL) return 0;
    }
    return 1;
}

// 헤더 값의 줄바꿈과 제어 문자가 업스트림 요청을 위조하지 못하게 합니다.
static int valid_header_value(const char *value) {
    for (const unsigned char *p = (const unsigned char *)value; *p; p++) {
        if ((*p < 0x20 && *p != '\t') || *p == 0x7f) return 0;
    }
    return 1;
}

// 요청의 Connection 헤더가 추가로 지정한 홉 사이 헤더도 걸러냅니다.
static int connection_names_header(const struct http_request *req,
                                   const char *name) {
    for (int i = 0; i < req->nheaders; i++) {
        if (strcasecmp(req->headers[i].name, "Connection") != 0) continue;

        char copy[1024];
        size_t length = strlen(req->headers[i].value);
        if (length >= sizeof(copy)) return 1;
        memcpy(copy, req->headers[i].value, length + 1);

        char *save = NULL;
        for (char *token = strtok_r(copy, ",", &save);
             token != NULL; token = strtok_r(NULL, ",", &save)) {
            while (*token == ' ' || *token == '\t') token++;
            size_t token_len = strlen(token);
            while (token_len > 0 &&
                   (token[token_len - 1] == ' ' || token[token_len - 1] == '\t')) {
                token[--token_len] = '\0';
            }
            if (strcasecmp(token, name) == 0) return 1;
        }
    }
    return 0;
}

// 프록시가 다음 연결 구간으로 넘기지 않는 hop-by-hop 헤더를 판별합니다.
static int is_hop_by_hop(const struct http_request *req, const char *name) {
    static const char *const names[] = {
        "Connection", "Keep-Alive", "Proxy-Connection",
        "Proxy-Authenticate", "Proxy-Authorization", "TE", "Trailer",
        "Transfer-Encoding", "Upgrade"
    };
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        if (strcasecmp(name, names[i]) == 0) return 1;
    }
    return connection_names_header(req, name);
}

// 포트가 있는 IPv6 호스트에는 대괄호를 붙여 Host 형식으로 만듭니다.
static int make_host_header(const char *host, const char *port,
                            char *out, size_t capacity) {
    int has_colon = strchr(host, ':') != NULL;
    int length = snprintf(out, capacity, has_colon ? "[%s]:%s" : "%s:%s",
                          host, port);
    return length < 0 || (size_t)length >= capacity ? -1 : 0;
}

// 목적지 정보를 사용해 안전한 HTTP 요청 헤더를 새로 만듭니다.
static char *build_request(const struct http_request *req,
                           const char *request_target,
                           const char *host_header,
                           size_t body_len, const char *client_ip,
                           size_t *header_len) {
    char *buffer = malloc(PROXY_HEADER_CAP);
    if (buffer == NULL) return NULL;
    size_t used = 0;

    if (append_format(buffer, PROXY_HEADER_CAP, &used,
                      "%s %s HTTP/1.1\r\n", req->method,
                      request_target) < 0 ||
        append_format(buffer, PROXY_HEADER_CAP, &used,
                      "Host: %s\r\n", host_header) < 0) {
        free(buffer);
        errno = EOVERFLOW;
        return NULL;
    }

    int had_content_length = 0;
    for (int i = 0; i < req->nheaders; i++) {
        const struct http_header *header = &req->headers[i];
        if (!valid_header_name(header->name) ||
            !valid_header_value(header->value)) {
            free(buffer);
            errno = EINVAL;
            return NULL;
        }
        if (strcasecmp(header->name, "Content-Length") == 0) {
            had_content_length = 1;
            continue;
        }
        if (strcasecmp(header->name, "Host") == 0 ||
            strcasecmp(header->name, "X-Forwarded-For") == 0 ||
            is_hop_by_hop(req, header->name)) {
            continue;
        }
        if (append_format(buffer, PROXY_HEADER_CAP, &used, "%s: %s\r\n",
                          header->name, header->value) < 0) {
            free(buffer);
            errno = EOVERFLOW;
            return NULL;
        }
    }

    if (had_content_length || body_len > 0) {
        if (append_format(buffer, PROXY_HEADER_CAP, &used,
                          "Content-Length: %zu\r\n", body_len) < 0) {
            free(buffer);
            errno = EOVERFLOW;
            return NULL;
        }
    }
    if (append_format(buffer, PROXY_HEADER_CAP, &used,
                      "X-Forwarded-For: %s\r\nConnection: close\r\n\r\n",
                      client_ip != NULL ? client_ip : "unknown") < 0) {
        free(buffer);
        errno = EOVERFLOW;
        return NULL;
    }

    *header_len = used;
    return buffer;
}

// 연결·요청 전송·응답 중계를 한곳에서 처리하고 만든 소켓을 항상 닫습니다.
static int proxy_exchange_connected(int client_fd,
                                    const struct http_request *req,
                                    const char *body, size_t body_len,
                                    int upstream_fd, const char *host_header,
                                    const char *request_target,
                                    const char *client_ip,
                                    int timeout_ms,
                                    struct cache_capture *capture) {
    size_t request_len = 0;
    char *request = build_request(req, request_target, host_header,
                                  body_len, client_ip, &request_len);
    if (request == NULL) {
        int saved_error = errno;
        server_log_errno("build proxy request", saved_error);
        close(upstream_fd);
        return resp_send_error(client_fd, 500) < 0 ? PROXY_FAILED : PROXY_HANDLED;
    }

    if (net_send_all(upstream_fd, request, request_len) < 0 ||
        (body_len > 0 && net_send_all(upstream_fd, body, body_len) < 0)) {
        int saved_error = errno;
        free(request);
        close(upstream_fd);
        server_log("upstream request send failed (errno=%d)\n", saved_error);
        return resp_send_error(client_fd, 502) < 0 ? PROXY_FAILED : PROXY_HANDLED;
    }
    free(request);

    char buffer[PROXY_IO_CAP];
    size_t relayed = 0;
    int result = PROXY_HANDLED;
    for (;;) {
        int ready;
        do {
            ready = net_wait(upstream_fd, POLLIN, timeout_ms);
        } while (ready < 0 && errno == EINTR);

        if (ready == 0) {
            if (relayed == 0) {
                result = resp_send_error(client_fd, 504) < 0 ?
                         PROXY_FAILED : PROXY_HANDLED;
            }
            break;
        }
        if (ready < 0) {
            int saved_error = errno;
            if (relayed == 0) {
                server_log("upstream wait failed (errno=%d)\n", saved_error);
                result = resp_send_error(client_fd, 502) < 0 ?
                         PROXY_FAILED : PROXY_HANDLED;
            }
            break;
        }

        ssize_t count = recv(upstream_fd, buffer, sizeof(buffer), 0);
        if (count < 0 && errno == EINTR) continue;
        if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            if (relayed == 0) {
                result = resp_send_error(client_fd, 504) < 0 ?
                         PROXY_FAILED : PROXY_HANDLED;
            }
            break;
        }
        if (count < 0) {
            int saved_error = errno;
            if (relayed == 0) {
                server_log("upstream receive failed (errno=%d)\n", saved_error);
                result = resp_send_error(client_fd, 502) < 0 ?
                         PROXY_FAILED : PROXY_HANDLED;
            }
            break;
        }
        if (count == 0) break;

        int send_result = capture != NULL ?
            cache_capture_write(capture, buffer, (size_t)count) :
            net_send_all(client_fd, buffer, (size_t)count);
        if (send_result < 0) {
            int saved_error = errno;
            if (saved_error != EPIPE && saved_error != ECONNRESET) {
                server_log("client response send failed (errno=%d)\n",
                           saved_error);
            }
            result = PROXY_FAILED;
            break;
        }
        relayed += (size_t)count;
    }

    close(upstream_fd);
    return result;
}

// 업스트림에 연결하고, 연결 실패를 502 또는 504 응답으로 돌려줍니다.
static int proxy_exchange(int client_fd, const struct http_request *req,
                          const char *body, size_t body_len,
                          const struct upstream *upstream,
                          const char *request_target, const char *client_ip,
                          int timeout_ms, struct cache_capture *capture) {
    int upstream_fd = net_connect(upstream->host, upstream->port, timeout_ms);
    if (upstream_fd < 0) {
        int saved_error = errno;
        int status = saved_error == ETIMEDOUT ? 504 : 502;
        server_log("upstream connect failed (errno=%d, status=%d)\n",
                   saved_error, status);
        return resp_send_error(client_fd, status) < 0 ? PROXY_FAILED : PROXY_HANDLED;
    }
    return proxy_exchange_connected(client_fd, req, body, body_len,
                                    upstream_fd, upstream->host_header,
                                    request_target, client_ip, timeout_ms,
                                    capture);
}

// 라우팅 규칙을 찾아 백엔드를 차례로 시도하고 응답을 그대로 전달합니다.
int proxy_reverse_cached(int client_fd, const struct http_request *req,
                         const char *body, size_t body_len,
                         const struct config *cfg, const char *client_ip,
                         struct cache_capture *capture) {
    struct route *route = route_match(cfg->routes, cfg->route_count,
                                      http_header_get(req, "Host"), req->target);
    if (route == NULL) return PROXY_NO_ROUTE;

    int timeout_ms = cfg->timeout_sec * 1000;
    int last_error = ECONNREFUSED;
    for (int attempt = 0; attempt < route->nbackends; attempt++) {
        const struct backend *backend = route_pick(route);
        if (backend == NULL) return PROXY_FAILED;

        struct upstream upstream;
        memset(&upstream, 0, sizeof(upstream));
        memcpy(upstream.host, backend->host, strlen(backend->host) + 1);
        memcpy(upstream.port, backend->port, strlen(backend->port) + 1);
        if (make_host_header(upstream.host, upstream.port,
                             upstream.host_header,
                             sizeof(upstream.host_header)) < 0) {
            return resp_send_error(client_fd, 500) < 0 ?
                   PROXY_FAILED : PROXY_HANDLED;
        }

        int upstream_fd = net_connect(upstream.host, upstream.port, timeout_ms);
        if (upstream_fd < 0) {
            last_error = errno;
            continue;
        }
        return proxy_exchange_connected(client_fd, req, body, body_len,
                                        upstream_fd, upstream.host_header,
                                        req->target, client_ip, timeout_ms,
                                        capture);
    }

    int status = last_error == ETIMEDOUT ? 504 : 502;
    server_log("all route backends failed (errno=%d, status=%d)\n",
               last_error, status);
    return resp_send_error(client_fd, status) < 0 ? PROXY_FAILED : PROXY_HANDLED;
}

int proxy_reverse(int client_fd, const struct http_request *req,
                  const char *body, size_t body_len, const struct config *cfg,
                  const char *client_ip) {
    return proxy_reverse_cached(client_fd, req, body, body_len, cfg, client_ip,
                                NULL);
}

// http:// authority와 경로를 나눠 포워드 프록시 목적지를 구성합니다.
static int parse_absolute_target(const char *target, struct upstream *upstream,
                                  char *origin, size_t origin_capacity) {
    if (strncmp(target, "https://", 8) == 0) return 400;
    if (strncmp(target, "http://", 7) != 0) return 400;

    const char *authority = target + 7;
    const char *path = strpbrk(authority, "/?#");
    size_t authority_len = path != NULL ? (size_t)(path - authority) :
                          strlen(authority);
    if (authority_len == 0 || authority_len >= sizeof(upstream->host_header))
        return 400;

    char authority_copy[272];
    memcpy(authority_copy, authority, authority_len);
    authority_copy[authority_len] = '\0';
    if (strchr(authority_copy, '@') != NULL) return 400;

    const char *host_start = authority_copy;
    const char *host_end = NULL;
    const char *port = "80";
    if (authority_copy[0] == '[') {
        host_start++;
        host_end = strchr(host_start, ']');
        if (host_end == NULL) return 400;
        if (host_end[1] == ':') port = host_end + 2;
        else if (host_end[1] != '\0') return 400;
    } else {
        const char *colon = strchr(authority_copy, ':');
        host_end = colon != NULL ? colon : authority_copy + authority_len;
        if (colon != NULL) port = colon + 1;
    }
    size_t host_len = (size_t)(host_end - host_start);
    if (host_len == 0 || host_len >= sizeof(upstream->host) ||
        strlen(port) >= sizeof(upstream->port)) return 400;
    for (const char *p = port; *p; p++) {
        if (!isdigit((unsigned char)*p)) return 400;
    }
    char *port_end = NULL;
    errno = 0;
    long port_number = strtol(port, &port_end, 10);
    if (errno == ERANGE || port_end == port || *port_end != '\0' ||
        port_number < 1 || port_number > 65535) return 400;

    memcpy(upstream->host, host_start, host_len);
    upstream->host[host_len] = '\0';
    memcpy(upstream->port, port, strlen(port) + 1);
    memcpy(upstream->host_header, authority_copy, authority_len + 1);

    if (path == NULL || *path == '#') {
        if (origin_capacity < 2) return 400;
        memcpy(origin, "/", 2);
    } else if (*path == '?') {
        int n = snprintf(origin, origin_capacity, "/%s", path);
        if (n < 0 || (size_t)n >= origin_capacity) return 414;
    } else if (*path == '#') {
        if (origin_capacity < 2) return 414;
        memcpy(origin, "/", 2);
    } else {
        const char *fragment = strchr(path, '#');
        size_t path_len = fragment != NULL ? (size_t)(fragment - path) : strlen(path);
        if (path_len == 0 || path_len >= origin_capacity) return 414;
        memcpy(origin, path, path_len);
        origin[path_len] = '\0';
    }
    return 0;
}

// -f 모드에서 absolute-form 요청을 origin-form으로 바꾸어 전달합니다.
int proxy_forward_cached(int client_fd, const struct http_request *req,
                         const char *body, size_t body_len, int timeout_sec,
                         const char *client_ip,
                         struct cache_capture *capture) {
    struct upstream upstream;
    memset(&upstream, 0, sizeof(upstream));
    char origin[2048];
    int parse_result = parse_absolute_target(req->target, &upstream,
                                             origin, sizeof(origin));
    if (parse_result != 0) {
        return resp_send_error(client_fd, parse_result) < 0 ?
               PROXY_FAILED : PROXY_HANDLED;
    }
    return proxy_exchange(client_fd, req, body, body_len, &upstream, origin,
                          client_ip, timeout_sec * 1000, capture);
}

int proxy_forward(int client_fd, const struct http_request *req,
                  const char *body, size_t body_len, int timeout_sec,
                  const char *client_ip) {
    return proxy_forward_cached(client_fd, req, body, body_len, timeout_sec,
                                client_ip, NULL);
}
