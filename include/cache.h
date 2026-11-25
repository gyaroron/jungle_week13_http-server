#ifndef CACHE_H
#define CACHE_H

#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include "http.h"

#define CACHE_KEY_MAX 2048
#define CACHE_ITEM_MAX (256U * 1024U)
#define CACHE_HEADER_MAX (64U * 1024U)
#define CACHE_SOURCE_MAX 4096

struct cache_capture {
    int client_fd;
    char key[CACHE_KEY_MAX];
    unsigned char *pending;
    size_t pending_len;
    size_t pending_cap;
    unsigned char *response;
    size_t response_len;
    size_t response_cap;
    int header_sent;
    int status;
    int disabled;
    int cacheable_response;
    size_t header_length;
    size_t expected_length;
    size_t body_length;
    size_t ttl_seconds;
    uint64_t generation;
    int keep_alive;
    int has_file_source;
    char file_source[CACHE_SOURCE_MAX];
    struct stat file_source_stat;
};

/* Make an absolute-URL cache key. Return 1 when cacheable, 0 when bypassed. */
int cache_key_for_request(const struct http_request *req,
                          char *key, size_t key_capacity);

/* Return 1 after sending a HIT, 0 on a miss or cache failure, -1 on send error.
 * 저장 응답은 항상 Content-Length가 있으므로 keep_alive를 그대로 표시할 수 있습니다. */
int cache_lookup_send(int client_fd, const char *key, int keep_alive);

/* Capture a GET response while streaming it; only complete 200 responses are stored. */
int cache_capture_begin(struct cache_capture *capture, int client_fd,
                        const char *key);
int cache_capture_set_file_source(struct cache_capture *capture,
                                 const char *path,
                                 const struct stat *source_stat);
/* 길이가 정해진 응답(정적 파일)만 MISS 응답에 keep-alive를 표시하도록 켭니다. */
int cache_capture_set_keep_alive(struct cache_capture *capture, int keep_alive);
int cache_capture_write(struct cache_capture *capture,
                        const void *data, size_t length);
int cache_capture_finish(struct cache_capture *capture);
void cache_capture_abort(struct cache_capture *capture);
/* 비-GET 요청의 앞뒤에서 부릅니다. 사이 구간에는 GET이 캐시를 건너뜁니다.
 * 잠금을 요청 처리 시간 동안 잡지 않으므로 느린 클라이언트가 다른 요청을 막지 않습니다. */
void cache_write_begin(void);
void cache_write_end(void);
void cache_invalidate_all(void);
void cache_destroy(void);

#endif
