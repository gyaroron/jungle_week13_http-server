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
    int has_file_source;
    char file_source[CACHE_SOURCE_MAX];
    struct stat file_source_stat;
};

/* Make an absolute-URL cache key. Return 1 when cacheable, 0 when bypassed. */
int cache_key_for_request(const struct http_request *req,
                          char *key, size_t key_capacity);

/* Return 1 after sending a HIT, 0 on a miss or cache failure, -1 on send error. */
int cache_lookup_send(int client_fd, const char *key);

/* Capture a GET response while streaming it; only complete 200 responses are stored. */
int cache_capture_begin(struct cache_capture *capture, int client_fd,
                        const char *key);
int cache_capture_set_file_source(struct cache_capture *capture,
                                 const char *path,
                                 const struct stat *source_stat);
int cache_capture_write(struct cache_capture *capture,
                        const void *data, size_t length);
int cache_capture_finish(struct cache_capture *capture);
void cache_capture_abort(struct cache_capture *capture);
/* GET은 공유 읽기 잠금, 다른 메서드는 단독 변경 잠금을 잡습니다. */
int cache_request_lock(int is_write);
int cache_request_unlock(void);
void cache_invalidate_all(void);
void cache_destroy(void);

#endif
