#include "cache.h"

#include "net.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#define CACHE_ENTRY_LIMIT 64
#define CACHE_TOTAL_LIMIT (8U * 1024U * 1024U)
// 응답에 더 긴 유효 기간이 있어도 캐시는 최대 30초까지만 보관합니다.
#define CACHE_TTL_SECONDS 30

struct cache_entry {
    char key[CACHE_KEY_MAX];
    unsigned char *response;
    size_t length;
    struct timespec expires_at;
    struct cache_entry *next;
};

static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_rwlock_t cache_consistency_lock = PTHREAD_RWLOCK_INITIALIZER;
static struct cache_entry *cache_head;
static struct cache_entry *cache_tail;
static size_t cache_entries;
static size_t cache_bytes;
static uint64_t cache_generation;

static int time_reached(const struct timespec *now,
                        const struct timespec *deadline) {
    return now->tv_sec > deadline->tv_sec ||
           (now->tv_sec == deadline->tv_sec &&
            now->tv_nsec >= deadline->tv_nsec);
}

// monotonic clock 기준 만료 시각이 지난 항목을 잠금 안에서 제거합니다.
static void remove_expired_locked(const struct timespec *now) {
    struct cache_entry **link = &cache_head;
    cache_tail = NULL;
    while (*link != NULL) {
        struct cache_entry *entry = *link;
        if (time_reached(now, &entry->expires_at)) {
            *link = entry->next;
            cache_entries--;
            cache_bytes -= entry->length;
            free(entry->response);
            free(entry);
        } else {
            cache_tail = entry;
            link = &entry->next;
        }
    }
}

static int span_equals(const unsigned char *text, size_t length,
                       const char *word) {
    return strlen(word) == length &&
           strncasecmp((const char *)text, word, length) == 0;
}

static int parse_decimal_span(const unsigned char *text, size_t length,
                              size_t *value) {
    if (length == 0) return -1;
    size_t parsed = 0;
    for (size_t i = 0; i < length; i++) {
        if (text[i] < '0' || text[i] > '9') return -1;
        unsigned digit = (unsigned)(text[i] - '0');
        if (parsed > (SIZE_MAX - digit) / 10) return -1;
        parsed = parsed * 10 + digit;
    }
    *value = parsed;
    return 0;
}

static void apply_cache_control(const unsigned char *value, size_t length,
                                size_t *ttl_seconds, int *safe_to_store) {
    size_t max_age = 0;
    size_t shared_max_age = 0;
    int has_max_age = 0;
    int has_shared_max_age = 0;
    size_t pos = 0;
    // 원 서버가 max-age/s-maxage를 더 짧게 지정하면 그 값을 우선합니다.
    while (pos < length) {
        while (pos < length && (value[pos] == ',' || value[pos] == ' ' ||
                                value[pos] == '\t')) pos++;
        size_t end = pos;
        while (end < length && value[end] != ',') end++;
        size_t left = pos, right = end;
        while (left < right && (value[left] == ' ' || value[left] == '\t')) left++;
        while (right > left && (value[right - 1] == ' ' ||
                                value[right - 1] == '\t')) right--;
        size_t equals = left;
        while (equals < right && value[equals] != '=') equals++;
        size_t name_end = equals;
        while (name_end > left && (value[name_end - 1] == ' ' ||
                                   value[name_end - 1] == '\t')) name_end--;
        if (span_equals(value + left, name_end - left, "private") ||
            span_equals(value + left, name_end - left, "no-store") ||
            span_equals(value + left, name_end - left, "no-cache") ||
            span_equals(value + left, name_end - left, "must-revalidate") ||
            span_equals(value + left, name_end - left, "proxy-revalidate")) {
            *safe_to_store = 0;
        }
        if (equals < right) {
            size_t value_start = equals + 1;
            while (value_start < right && (value[value_start] == ' ' ||
                                           value[value_start] == '\t')) value_start++;
            size_t value_end = right;
            while (value_end > value_start && (value[value_end - 1] == ' ' ||
                                               value[value_end - 1] == '\t')) value_end--;
            if (value_end - value_start >= 2 && value[value_start] == '"' &&
                value[value_end - 1] == '"') {
                value_start++;
                value_end--;
            }
            size_t parsed_age;
            if (span_equals(value + left, name_end - left, "max-age")) {
                if (has_max_age || parse_decimal_span(value + value_start,
                                       value_end - value_start,
                                       &parsed_age) < 0) {
                    *safe_to_store = 0;
                } else {
                    max_age = parsed_age;
                    has_max_age = 1;
                }
            } else if (span_equals(value + left, name_end - left, "s-maxage")) {
                if (has_shared_max_age || parse_decimal_span(value + value_start,
                                       value_end - value_start,
                                       &parsed_age) < 0) {
                    *safe_to_store = 0;
                } else {
                    shared_max_age = parsed_age;
                    has_shared_max_age = 1;
                }
            }
        }
        pos = end < length ? end + 1 : length;
    }
    if (has_shared_max_age || has_max_age) {
        size_t origin_age = has_shared_max_age ? shared_max_age : max_age;
        if (origin_age < *ttl_seconds) *ttl_seconds = origin_age;
    }
}

static void remove_oldest_locked(void) {
    struct cache_entry *oldest = cache_head;
    if (oldest == NULL) return;
    cache_head = oldest->next;
    if (cache_head == NULL) cache_tail = NULL;
    cache_entries--;
    cache_bytes -= oldest->length;
    free(oldest->response);
    free(oldest);
}

static int header_end(const unsigned char *data, size_t length, size_t *end) {
    if (length < 4) return 0;
    for (size_t i = 0; i + 3 < length; i++) {
        if (memcmp(data + i, "\r\n\r\n", 4) == 0) {
            *end = i + 4;
            return 1;
        }
    }
    return 0;
}

static int forbidden_request_header(const struct http_request *req) {
    static const char *const names[] = {
        "Authorization", "Cookie", "Range", "If-Modified-Since",
        "If-None-Match", "Cache-Control", "Pragma"
    };
    for (int i = 0; i < req->nheaders; i++) {
        for (size_t j = 0; j < sizeof(names) / sizeof(names[0]); j++) {
            if (strcasecmp(req->headers[i].name, names[j]) == 0) return 1;
        }
    }
    return 0;
}

int cache_key_for_request(const struct http_request *req,
                          char *key, size_t capacity) {
    if (req == NULL || key == NULL || capacity == 0 ||
        strcmp(req->method, "GET") != 0 || forbidden_request_header(req)) {
        return 0;
    }
    int length;
    if (strncmp(req->target, "http://", 7) == 0) {
        length = snprintf(key, capacity, "%s", req->target);
    } else {
        const char *host = http_header_get(req, "Host");
        if (host == NULL || *host == '\0' || req->target[0] != '/') return 0;
        length = snprintf(key, capacity, "http://%s%s", host, req->target);
    }
    if (length < 0 || (size_t)length >= capacity) return 0;
    return 1;
}

int cache_request_lock(int is_write) {
    int result = is_write ? pthread_rwlock_wrlock(&cache_consistency_lock) :
                            pthread_rwlock_rdlock(&cache_consistency_lock);
    if (result != 0) {
        errno = result;
        return -1;
    }
    return 0;
}

int cache_request_unlock(void) {
    int result = pthread_rwlock_unlock(&cache_consistency_lock);
    if (result != 0) {
        errno = result;
        return -1;
    }
    return 0;
}

static int transformed_header(const unsigned char *raw, size_t raw_length,
                              const char *cache_status,
                              unsigned char **out, size_t *out_length,
                              int *status, size_t *content_length,
                              int *has_content_length, int *safe_to_store,
                              size_t *ttl_seconds) {
    size_t end;
    if (!header_end(raw, raw_length, &end)) return 0;
    size_t first_line = 0;
    while (first_line + 1 < end &&
           !(raw[first_line] == '\r' && raw[first_line + 1] == '\n')) {
        first_line++;
    }
    if (first_line + 1 >= end || first_line < 12 ||
        memcmp(raw, "HTTP/", 5) != 0) return -1;
    *status = 0;
    for (size_t i = 9; i < first_line && raw[i] >= '0' && raw[i] <= '9'; i++) {
        *status = *status * 10 + (raw[i] - '0');
    }
    *content_length = 0;
    *has_content_length = 0;
    *safe_to_store = 1;
    *ttl_seconds = CACHE_TTL_SECONDS;
    size_t capacity = end + 128;
    unsigned char *built = malloc(capacity);
    if (built == NULL) return -1;
    size_t used = 0;
    memcpy(built, raw, first_line + 2);
    used = first_line + 2;

    size_t pos = used;
    while (pos + 1 < end) {
        size_t line_end = pos;
        while (line_end + 1 < end &&
               !(raw[line_end] == '\r' && raw[line_end + 1] == '\n')) {
            line_end++;
        }
        if (line_end == pos) break;
        size_t colon = pos;
        while (colon < line_end && raw[colon] != ':') colon++;
        size_t name_len = colon - pos;
        if (colon < line_end && name_len == 14 &&
            strncasecmp((const char *)raw + pos, "Content-Length", name_len) == 0) {
            char number[32];
            size_t value_start = colon + 1;
            while (value_start < line_end && raw[value_start] == ' ') value_start++;
            size_t value_len = line_end - value_start;
            if (value_len == 0 || value_len >= sizeof(number)) *safe_to_store = 0;
            else {
                memcpy(number, raw + value_start, value_len);
                number[value_len] = '\0';
                char *tail = NULL;
                errno = 0;
                unsigned long long parsed = strtoull(number, &tail, 10);
                if (errno != 0 || tail == number || *tail != '\0' ||
                    parsed > CACHE_ITEM_MAX) *safe_to_store = 0;
                else {
                    *content_length = (size_t)parsed;
                    *has_content_length = 1;
                }
            }
        }
        if (colon < line_end &&
            ((name_len == 10 && strncasecmp((const char *)raw + pos,
                                            "Connection", 10) == 0) ||
             (name_len == 7 && strncasecmp((const char *)raw + pos,
                                           "X-Cache", 7) == 0))) {
            // Connection and X-Cache are regenerated for each response.
        } else {
            memcpy(built + used, raw + pos, line_end + 2 - pos);
            used += line_end + 2 - pos;
        }
        if (colon < line_end &&
            ((name_len == 17 && strncasecmp((const char *)raw + pos,
                                            "Transfer-Encoding", 17) == 0) ||
             (name_len == 4 && strncasecmp((const char *)raw + pos,
                                           "Vary", 4) == 0) ||
             (name_len == 10 && strncasecmp((const char *)raw + pos,
                                            "Set-Cookie", 10) == 0) ||
             (name_len == 7 && strncasecmp((const char *)raw + pos,
                                           "Expires", 7) == 0))) {
            *safe_to_store = 0;
        }
        if (colon < line_end && name_len == 13 &&
            strncasecmp((const char *)raw + pos, "Cache-Control", 13) == 0) {
            size_t value_len = line_end - colon - 1;
            apply_cache_control(raw + colon + 1, value_len,
                                ttl_seconds, safe_to_store);
        }
        pos = line_end + 2;
    }
    int added = snprintf((char *)built + used, capacity - used,
                         "X-Cache: %s\r\nConnection: close\r\n\r\n",
                         cache_status);
    if (added < 0 || (size_t)added >= capacity - used) {
        free(built);
        return -1;
    }
    used += (size_t)added;
    *out = built;
    *out_length = used;
    return 1;
}

static int send_transformed(int fd, const unsigned char *raw, size_t length,
                            const char *status) {
    unsigned char *header = NULL;
    size_t header_length = 0, expected = 0, ttl_seconds = 0;
    int code, has_length, safe;
    int result = transformed_header(raw, length, status, &header,
                                    &header_length, &code, &expected,
                                    &has_length, &safe, &ttl_seconds);
    (void)expected;
    (void)has_length;
    (void)safe;
    (void)ttl_seconds;
    if (result != 1) return -1;
    size_t raw_end = 0;
    if (!header_end(raw, length, &raw_end)) {
        free(header);
        return -1;
    }
    int sent = net_send_all(fd, header, header_length);
    free(header);
    if (sent < 0) return -1;
    return length > raw_end ? net_send_all(fd, raw + raw_end, length - raw_end) : 0;
}

int cache_lookup_send(int client_fd, const char *key) {
    if (key == NULL) return 0;
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) < 0) return 0;
    unsigned char *copy = NULL;
    size_t length = 0;
    pthread_mutex_lock(&cache_mutex);
    remove_expired_locked(&now);
    for (struct cache_entry *entry = cache_head; entry != NULL; entry = entry->next) {
        if (strcmp(entry->key, key) == 0) {
            copy = malloc(entry->length);
            if (copy != NULL) {
                memcpy(copy, entry->response, entry->length);
                length = entry->length;
            }
            break;
        }
    }
    pthread_mutex_unlock(&cache_mutex);
    if (copy == NULL) return 0;
    int result = send_transformed(client_fd, copy, length, "HIT");
    free(copy);
    return result < 0 ? -1 : 1;
}

static int append_bytes(unsigned char **buffer, size_t *used, size_t *capacity,
                        const void *data, size_t length, size_t limit) {
    if (length > limit - *used) return -1;
    size_t needed = *used + length;
    if (needed > *capacity) {
        size_t next = *capacity == 0 ? 1024 : *capacity;
        while (next < needed) next = next > limit / 2 ? limit : next * 2;
        unsigned char *grown = realloc(*buffer, next);
        if (grown == NULL) return -1;
        *buffer = grown;
        *capacity = next;
    }
    memcpy(*buffer + *used, data, length);
    *used = needed;
    return 0;
}

int cache_capture_begin(struct cache_capture *capture, int client_fd,
                        const char *key) {
    if (capture == NULL || key == NULL) return -1;
    memset(capture, 0, sizeof(*capture));
    capture->client_fd = client_fd;
    memcpy(capture->key, key, strlen(key) + 1);
    // 쓰기 요청이 사이에 끼어들었는지 나중에 저장 직전에 비교합니다.
    pthread_mutex_lock(&cache_mutex);
    capture->generation = cache_generation;
    pthread_mutex_unlock(&cache_mutex);
    return 0;
}

int cache_capture_write(struct cache_capture *capture,
                        const void *data, size_t length) {
    if (capture == NULL || data == NULL || length == 0) return 0;
    const unsigned char *bytes = data;
    if (capture->disabled) {
        return net_send_all(capture->client_fd, bytes, length);
    }
    if (!capture->header_sent) {
        if (append_bytes(&capture->pending, &capture->pending_len,
                         &capture->pending_cap, bytes, length,
                         CACHE_HEADER_MAX) < 0) {
            if (capture->pending_len > 0 &&
                net_send_all(capture->client_fd, capture->pending,
                             capture->pending_len) < 0) return -1;
            if (net_send_all(capture->client_fd, bytes, length) < 0) return -1;
            capture->disabled = 1;
            capture->header_sent = 1;
            free(capture->pending);
            capture->pending = NULL;
            capture->pending_len = capture->pending_cap = 0;
            return 0;
        }
        size_t end;
        if (!header_end(capture->pending, capture->pending_len, &end)) return 0;
        capture->header_length = end;
        unsigned char *header = NULL;
        size_t transformed_len = 0;
        int has_length, safe;
        size_t response_ttl = 0;
        int parsed = transformed_header(capture->pending, end, "MISS", &header,
                                        &transformed_len, &capture->status,
                                        &capture->expected_length,
                                        &has_length, &safe, &response_ttl);
        capture->ttl_seconds = response_ttl;
        capture->cacheable_response = parsed == 1 && safe && has_length &&
                                      capture->status == 200 && response_ttl > 0;
        if (parsed != 1) {
            free(header);
            if (net_send_all(capture->client_fd, capture->pending,
                             capture->pending_len) < 0) return -1;
            capture->disabled = 1;
            capture->header_sent = 1;
            free(capture->pending);
            capture->pending = NULL;
            capture->pending_len = capture->pending_cap = 0;
            return 0;
        }
        if (net_send_all(capture->client_fd, header, transformed_len) < 0) {
            free(header);
            return -1;
        }
        free(header);
        capture->header_sent = 1;
        capture->body_length = capture->pending_len - end;
        if (append_bytes(&capture->response, &capture->response_len,
                         &capture->response_cap, capture->pending,
                         capture->pending_len, CACHE_ITEM_MAX) < 0) {
            capture->cacheable_response = 0;
        }
        if (capture->body_length > 0 &&
            net_send_all(capture->client_fd, capture->pending + end,
                         capture->body_length) < 0) return -1;
        free(capture->pending);
        capture->pending = NULL;
        capture->pending_len = capture->pending_cap = 0;
        return 0;
    }

    capture->body_length += length;
    if (capture->cacheable_response &&
        append_bytes(&capture->response, &capture->response_len,
                     &capture->response_cap, bytes, length,
                     CACHE_ITEM_MAX) < 0) {
        capture->cacheable_response = 0;
    }
    return net_send_all(capture->client_fd, bytes, length);
}

int cache_capture_finish(struct cache_capture *capture) {
    if (capture == NULL) return -1;
    if (!capture->header_sent && capture->pending_len > 0) {
        // 헤더가 완성되지 않은 응답은 MISS 표시 없이 그대로 전달합니다.
        if (net_send_all(capture->client_fd, capture->pending,
                         capture->pending_len) < 0) {
            cache_capture_abort(capture);
            return -1;
        }
    }
    if (capture->cacheable_response && capture->expected_length == capture->body_length &&
        capture->response_len <= CACHE_ITEM_MAX) {
        struct timespec now;
        struct cache_entry *entry = calloc(1, sizeof(*entry));
        unsigned char *saved = malloc(capture->response_len);
        if (entry != NULL && saved != NULL &&
            clock_gettime(CLOCK_MONOTONIC, &now) == 0) {
            memcpy(entry->key, capture->key, strlen(capture->key) + 1);
            memcpy(saved, capture->response, capture->response_len);
            entry->response = saved;
            saved = NULL;
            entry->length = capture->response_len;
            entry->expires_at = now;
            entry->expires_at.tv_sec += (time_t)capture->ttl_seconds;
            pthread_mutex_lock(&cache_mutex);
            remove_expired_locked(&now);
            if (capture->generation == cache_generation) {
                struct cache_entry **link = &cache_head;
                while (*link != NULL) {
                    if (strcmp((*link)->key, entry->key) == 0) {
                        struct cache_entry *old = *link;
                        *link = old->next;
                        if (cache_tail == old) cache_tail = NULL;
                        cache_entries--;
                        cache_bytes -= old->length;
                        free(old->response);
                        free(old);
                        break;
                    }
                    link = &(*link)->next;
                }
                if (cache_tail == NULL && cache_head != NULL) {
                    cache_tail = cache_head;
                    while (cache_tail->next != NULL) cache_tail = cache_tail->next;
                }
                while (cache_head != NULL &&
                       (cache_entries >= CACHE_ENTRY_LIMIT ||
                        cache_bytes + entry->length > CACHE_TOTAL_LIMIT)) {
                    remove_oldest_locked();
                }
                if (entry->length <= CACHE_TOTAL_LIMIT) {
                    if (cache_tail == NULL) cache_head = cache_tail = entry;
                    else { cache_tail->next = entry; cache_tail = entry; }
                    cache_entries++;
                    cache_bytes += entry->length;
                    entry = NULL;
                }
            }
            pthread_mutex_unlock(&cache_mutex);
        }
        if (entry != NULL) {
            free(entry->response);
            free(entry);
        }
        free(saved);
    }
    cache_capture_abort(capture);
    return 0;
}

void cache_capture_abort(struct cache_capture *capture) {
    if (capture == NULL) return;
    free(capture->pending);
    free(capture->response);
    memset(capture, 0, sizeof(*capture));
}

void cache_invalidate_all(void) {
    pthread_mutex_lock(&cache_mutex);
    while (cache_head != NULL) remove_oldest_locked();
    // 진행 중인 이전 GET 응답이 변경 뒤에 낡은 항목을 저장하지 못하게 합니다.
    cache_generation++;
    pthread_mutex_unlock(&cache_mutex);
}

void cache_destroy(void) {
    pthread_mutex_lock(&cache_mutex);
    while (cache_head != NULL) remove_oldest_locked();
    pthread_mutex_unlock(&cache_mutex);
}
