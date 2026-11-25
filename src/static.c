#define _DARWIN_C_SOURCE
#define _XOPEN_SOURCE 700

#include "static.h"

#include "cache.h"
#include "mime.h"
#include "net.h"
#include "response.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int hex_value(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

static int send_error_result(int fd, int status) {
    return resp_send_error(fd, status) < 0 ? -1 : 1;
}

// 경로의 %XX를 해독하고, 루트 이탈 조각이나 NUL 바이트를 거부합니다.
static int decode_target(const char *target, char *path, size_t capacity) {
    const char *query = strchr(target, '?');
    size_t input_len = query != NULL ? (size_t)(query - target) : strlen(target);
    if (input_len == 0 || target[0] != '/') return 400;

    size_t out = 0;
    for (size_t i = 0; i < input_len; i++) {
        unsigned char value = (unsigned char)target[i];
        if (target[i] == '%') {
            if (i + 2 >= input_len) return 400;
            int hi = hex_value(target[i + 1]);
            int lo = hex_value(target[i + 2]);
            if (hi < 0 || lo < 0) return 400;
            value = (unsigned char)((hi << 4) | lo);
            i += 2;
            if (value == 0) return 400;
        }
        if (out + 1 >= capacity) return 414;
        path[out++] = (char)value;
    }
    path[out] = '\0';

    // 인코딩을 푼 다음 각 경로 조각이 정확히 ".."인지 검사합니다.
    const char *part = path;
    while (*part != '\0') {
        while (*part == '/') part++;
        const char *end = strchr(part, '/');
        size_t length = end != NULL ? (size_t)(end - part) : strlen(part);
        if (length == 2 && part[0] == '.' && part[1] == '.') return 403;
        if (end == NULL) break;
        part = end + 1;
    }
    return 0;
}

static int path_is_inside(const char *root, const char *file) {
    size_t root_len = strlen(root);
    if (strncmp(root, file, root_len) != 0) return 0;
    return file[root_len] == '\0' || file[root_len] == '/';
}

int static_serve_cached(int fd, const struct config *cfg,
                        const struct http_request *req,
                        struct cache_capture *capture) {
    if (strcmp(req->method, "GET") != 0) return send_error_result(fd, 405);

    char decoded[PATH_MAX];
    int decode_result = decode_target(req->target, decoded, sizeof(decoded));
    if (decode_result != 0) return send_error_result(fd, decode_result);

    size_t path_len = strlen(decoded);
    if (path_len > 0 && decoded[path_len - 1] == '/') {
        static const char index_name[] = "index.html";
        if (path_len + sizeof(index_name) > sizeof(decoded)) {
            return send_error_result(fd, 414);
        }
        memcpy(decoded + path_len, index_name, sizeof(index_name));
    }

    char root[PATH_MAX];
    if (realpath(cfg->root, root) == NULL) {
        return send_error_result(fd, 500);
    }
    char candidate[PATH_MAX];
    int candidate_len = snprintf(candidate, sizeof(candidate), "%s/%s",
                                 root, decoded[0] == '/' ? decoded + 1 : decoded);
    if (candidate_len < 0 || (size_t)candidate_len >= sizeof(candidate)) {
        return send_error_result(fd, 414);
    }

    char file_path[PATH_MAX];
    if (realpath(candidate, file_path) == NULL) {
        if (errno == ENOENT) return send_error_result(fd, 404);
        if (errno == EACCES) return send_error_result(fd, 403);
        return send_error_result(fd, 500);
    }
    if (!path_is_inside(root, file_path)) return send_error_result(fd, 403);

    struct stat file_stat;
    if (stat(file_path, &file_stat) < 0) {
        if (errno == ENOENT) return send_error_result(fd, 404);
        if (errno == EACCES) return send_error_result(fd, 403);
        return send_error_result(fd, 500);
    }
    if (!S_ISREG(file_stat.st_mode) || file_stat.st_size < 0 || file_stat.st_size > LONG_MAX) {
        return send_error_result(fd, 403);
    }

    int file_fd = open(file_path, O_RDONLY);
    if (file_fd < 0) {
        if (errno == EACCES) return send_error_result(fd, 403);
        if (errno == ENOENT) return send_error_result(fd, 404);
        return send_error_result(fd, 500);
    }
    if (fstat(file_fd, &file_stat) < 0 || !S_ISREG(file_stat.st_mode)) {
        close(file_fd);
        return send_error_result(fd, 403);
    }
    if (capture != NULL) {
        // 파일 경로와 이 시점의 파일 정보를 저장해 다음 HIT 때 원본 변경을 검사합니다.
        (void)cache_capture_set_file_source(capture, candidate, &file_stat);
        // 정적 응답은 Content-Length가 정확하므로 클라이언트 요청대로 연결을 유지합니다.
        (void)cache_capture_set_keep_alive(capture, req->keep_alive);
    }

    char response_head[1024];
    int response_head_len = resp_format_head(response_head,
                                             sizeof(response_head), 200,
                                             mime_from_path(file_path),
                                             (long)file_stat.st_size,
                                             req->keep_alive);
    int head_result = response_head_len < 0 ? -1 :
        (capture != NULL ?
         cache_capture_write(capture, response_head,
                             (size_t)response_head_len) :
         net_send_all(fd, response_head, (size_t)response_head_len));
    if (head_result < 0) {
        close(file_fd);
        return -1;
    }

    char buffer[16384];
    int result = 0;
    for (;;) {
        ssize_t n = read(file_fd, buffer, sizeof(buffer));
        if (n < 0 && errno == EINTR) continue;
        if (n < 0) {
            result = -1;
            break;
        }
        if (n == 0) break;
        int send_result = capture != NULL ?
            cache_capture_write(capture, buffer, (size_t)n) :
            net_send_all(fd, buffer, (size_t)n);
        if (send_result < 0) {
            result = -1;
            break;
        }
    }
    close(file_fd);
    return result;
}

int static_serve(int fd, const struct config *cfg, const struct http_request *req) {
    return static_serve_cached(fd, cfg, req, NULL);
}
