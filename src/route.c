#include "route.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define ROUTE_LINE_CAP 4096

// 1부터 65535까지의 숫자만 포트 문자열로 받아들입니다.
static int valid_port(const char *port) {
    if (port == NULL || *port == '\0') return 0;
    char *end = NULL;
    errno = 0;
    long value = strtol(port, &end, 10);
    return errno != ERANGE && end != port && *end == '\0' &&
           value >= 1 && value <= 65535;
}

// host:port 또는 [IPv6]:port를 고정 크기 필드에 나눠 담습니다.
static int parse_backend(const char *text, struct backend *backend) {
    const char *host_start = text;
    const char *host_end = NULL;
    const char *port_start = NULL;

    if (text[0] == '[') {
        host_start++;
        host_end = strchr(host_start, ']');
        if (host_end == NULL || host_end[1] != ':') return -1;
        port_start = host_end + 2;
    } else {
        const char *colon = strrchr(text, ':');
        if (colon == NULL || colon == text) return -1;
        host_end = colon;
        port_start = colon + 1;
    }

    size_t host_len = (size_t)(host_end - host_start);
    size_t port_len = strlen(port_start);
    if (host_len == 0 || host_len >= sizeof(backend->host) ||
        port_len == 0 || port_len >= sizeof(backend->port) ||
        !valid_port(port_start)) {
        return -1;
    }

    memcpy(backend->host, host_start, host_len);
    backend->host[host_len] = '\0';
    memcpy(backend->port, port_start, port_len + 1);
    return 0;
}

// 실패한 로딩에서 이미 만든 mutex와 파일을 정리합니다.
static void destroy_loaded(struct route *routes, int count) {
    for (int i = 0; i < count; i++) {
        if (routes[i].mutex_ready) {
            pthread_mutex_destroy(&routes[i].mutex);
            routes[i].mutex_ready = 0;
        }
    }
}

// 각 줄을 호스트, 경로 접두사, 백엔드 목록으로 해석합니다.
int route_load(const char *path, struct route *routes, int max) {
    if (path == NULL || routes == NULL || max <= 0) {
        errno = EINVAL;
        return -1;
    }

    FILE *file = fopen(path, "r");
    if (file == NULL) return -1;

    char line[ROUTE_LINE_CAP];
    int count = 0;
    int failed = 0;
    while (fgets(line, sizeof(line), file) != NULL) {
        size_t length = strlen(line);
        if (length > 0 && line[length - 1] != '\n' && !feof(file)) {
            failed = 1;
            errno = EOVERFLOW;
            break;
        }
        if (length > 0 && line[length - 1] == '\n') line[--length] = '\0';
        if (length > 0 && line[length - 1] == '\r') line[--length] = '\0';

        char *cursor = line;
        while (isspace((unsigned char)*cursor)) cursor++;
        if (*cursor == '\0' || *cursor == '#') continue;
        if (count >= max) {
            failed = 1;
            errno = EOVERFLOW;
            break;
        }

        struct route *route = &routes[count];
        memset(route, 0, sizeof(*route));
        char *save = NULL;
        char *host = strtok_r(cursor, " \t", &save);
        char *prefix = strtok_r(NULL, " \t", &save);
        if (host == NULL || prefix == NULL || prefix[0] != '/' ||
            strlen(host) >= sizeof(route->host) ||
            strlen(prefix) >= sizeof(route->prefix)) {
            failed = 1;
            errno = EINVAL;
            break;
        }
        strcpy(route->host, host);
        strcpy(route->prefix, prefix);

        char *backend_text;
        while ((backend_text = strtok_r(NULL, " \t", &save)) != NULL) {
            if (route->nbackends >= ROUTE_BACKEND_MAX ||
                parse_backend(backend_text,
                              &route->backends[route->nbackends]) < 0) {
                failed = 1;
                errno = EINVAL;
                break;
            }
            route->nbackends++;
        }
        if (failed) break;
        if (route->nbackends == 0) {
            failed = 1;
            errno = EINVAL;
            break;
        }

        int result = pthread_mutex_init(&route->mutex, NULL);
        if (result != 0) {
            failed = 1;
            errno = result;
            break;
        }
        route->mutex_ready = 1;
        count++;
    }

    if (ferror(file)) failed = 1;
    fclose(file);
    if (failed) {
        destroy_loaded(routes, count);
        return -1;
    }
    return count;
}

// Host 헤더의 포트 부분을 제외해 규칙 파일의 호스트와 비교합니다.
static int host_matches(const char *rule_host, const char *request_host) {
    if (strcmp(rule_host, "*") == 0) return 1;
    if (request_host == NULL) return 0;

    char host[256];
    size_t length = strlen(request_host);
    if (request_host[0] == '[') {
        const char *closing = strchr(request_host, ']');
        if (closing == NULL) return 0;
        length = (size_t)(closing - request_host - 1);
        request_host++;
    } else {
        const char *colon = strchr(request_host, ':');
        if (colon != NULL) length = (size_t)(colon - request_host);
    }
    if (length == 0 || length >= sizeof(host)) return 0;
    memcpy(host, request_host, length);
    host[length] = '\0';
    return strcasecmp(rule_host, host) == 0;
}

// 가장 긴 경로 접두사를 우선하고, 길이가 같으면 호스트 일치 규칙을 우선합니다.
struct route *route_match(struct route *routes, int count,
                          const char *host, const char *target) {
    if (routes == NULL || count <= 0 || target == NULL) return NULL;

    struct route *best = NULL;
    size_t best_prefix = 0;
    int best_exact_host = 0;
    for (int i = 0; i < count; i++) {
        struct route *candidate = &routes[i];
        size_t prefix_len = strlen(candidate->prefix);
        if (!host_matches(candidate->host, host) ||
            strncmp(target, candidate->prefix, prefix_len) != 0) {
            continue;
        }

        int exact_host = strcmp(candidate->host, "*") != 0;
        if (best == NULL || prefix_len > best_prefix ||
            (prefix_len == best_prefix && exact_host > best_exact_host)) {
            best = candidate;
            best_prefix = prefix_len;
            best_exact_host = exact_host;
        }
    }
    return best;
}

// mutex 안에서 번호를 하나 증가시키고 그 번호에 해당하는 백엔드를 돌려줍니다.
const struct backend *route_pick(struct route *route) {
    if (route == NULL || route->nbackends <= 0) return NULL;
    if (pthread_mutex_lock(&route->mutex) != 0) return NULL;
    unsigned index = route->next % (unsigned)route->nbackends;
    route->next++;
    pthread_mutex_unlock(&route->mutex);
    return &route->backends[index];
}

// 각 규칙의 라운드 로빈 mutex를 제거합니다.
void route_destroy(struct route *routes, int count) {
    if (routes == NULL || count <= 0) return;
    destroy_loaded(routes, count);
}
