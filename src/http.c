#include "http.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// 요청 버퍼에 CRLFCRLF가 모두 도착했는지 확인합니다.
static int has_header_end(const char *buf, size_t len) {
    for (size_t i = 0; i + 3 < len; i++) {
        if (buf[i] == '\r' && buf[i + 1] == '\n' &&
            buf[i + 2] == '\r' && buf[i + 3] == '\n') {
            return 1;
        }
    }
    return 0;
}

// 결과 코드를 로그에서 읽을 이름으로 바꿉니다.
const char *http_result_name(enum http_result result) {
    switch (result) {
        case HTTP_OK: return "HTTP_OK";
        case HTTP_INCOMPLETE: return "HTTP_INCOMPLETE";
        case HTTP_BAD_REQUEST: return "HTTP_BAD_REQUEST";
        case HTTP_URI_TOO_LONG: return "HTTP_URI_TOO_LONG";
        case HTTP_HEADER_TOO_LARGE: return "HTTP_HEADER_TOO_LARGE";
        case HTTP_NOT_IMPLEMENTED: return "HTTP_NOT_IMPLEMENTED";
        case HTTP_VERSION_NOT_SUPPORTED: return "HTTP_VERSION_NOT_SUPPORTED";
        case HTTP_PAYLOAD_TOO_LARGE: return "HTTP_PAYLOAD_TOO_LARGE";
    }
    return "HTTP_BAD_REQUEST";
}

// 헤더 이름은 대소문자를 구분하지 않으므로 이름으로 값을 찾습니다.
const char *http_header_get(const struct http_request *req, const char *name) {
    if (req == NULL || name == NULL) {
        return NULL;
    }
    for (int i = 0; i < req->nheaders; i++) {
        if (strcasecmp(req->headers[i].name, name) == 0) {
            return req->headers[i].value;
        }
    }
    return NULL;
}

// 3-1 요청 줄에 이어 헤더 줄들을 배열에 담습니다.
enum http_result http_parse(const char *buf, size_t len, struct http_request *req) {
    if (buf == NULL || req == NULL) {
        return HTTP_BAD_REQUEST;
    }
    if (!has_header_end(buf, len)) {
        return HTTP_INCOMPLETE;
    }

    // 요청 줄의 끝을 찾습니다. 입력 버퍼는 수정하지 않습니다.
    size_t line_end = 0;
    while (line_end + 1 < len &&
           !(buf[line_end] == '\r' && buf[line_end + 1] == '\n')) {
        line_end++;
    }
    if (line_end + 1 >= len) {
        return HTTP_BAD_REQUEST;
    }

    // 요청 줄에는 method, target, version 사이의 공백이 정확히 두 개 있어야 합니다.
    size_t first_space = line_end;
    size_t second_space = line_end;
    for (size_t i = 0; i < line_end; i++) {
        if (buf[i] == ' ') {
            if (first_space == line_end) {
                first_space = i;
            } else if (second_space == line_end) {
                second_space = i;
            } else {
                return HTTP_BAD_REQUEST;
            }
        }
    }
    if (first_space == 0 || second_space == line_end ||
        second_space == first_space + 1 || second_space + 1 == line_end) {
        return HTTP_BAD_REQUEST;
    }

    size_t method_len = first_space;
    size_t target_start = first_space + 1;
    size_t target_len = second_space - target_start;
    size_t version_start = second_space + 1;
    size_t version_len = line_end - version_start;

    if (target_len >= sizeof(req->target)) {
        return HTTP_URI_TOO_LONG;
    }
    if (!((method_len == 3 && memcmp(buf, "GET", 3) == 0) ||
          (method_len == 4 && memcmp(buf, "POST", 4) == 0))) {
        return HTTP_NOT_IMPLEMENTED;
    }
    if (!((version_len == 8 && memcmp(buf + version_start, "HTTP/1.1", 8) == 0) ||
          (version_len == 8 && memcmp(buf + version_start, "HTTP/1.0", 8) == 0))) {
        return HTTP_VERSION_NOT_SUPPORTED;
    }

    memset(req, 0, sizeof(*req));
    memcpy(req->method, buf, method_len);
    req->method[method_len] = '\0';
    memcpy(req->target, buf + target_start, target_len);
    req->target[target_len] = '\0';
    memcpy(req->version, buf + version_start, version_len);
    req->version[version_len] = '\0';

    // 요청 줄 다음부터 빈 줄 전까지 한 헤더씩 읽습니다.
    size_t pos = line_end + 2;
    while (pos + 1 < len) {
        size_t end = pos;
        while (end + 1 < len && !(buf[end] == '\r' && buf[end + 1] == '\n')) {
            end++;
        }
        if (end + 1 >= len) {
            return HTTP_BAD_REQUEST;
        }
        if (end == pos) {
            req->header_len = end + 2;
            break;
        }

        size_t colon = pos;
        while (colon < end && buf[colon] != ':') {
            colon++;
        }
        if (colon == pos || colon == end) {
            return HTTP_BAD_REQUEST;
        }
        if (req->nheaders >= (int)(sizeof(req->headers) / sizeof(req->headers[0]))) {
            return HTTP_HEADER_TOO_LARGE;
        }

        size_t name_len = colon - pos;
        size_t value_start = colon + 1;
        size_t value_end = end;
        while (value_start < value_end &&
               (buf[value_start] == ' ' || buf[value_start] == '\t')) {
            value_start++;
        }
        while (value_end > value_start &&
               (buf[value_end - 1] == ' ' || buf[value_end - 1] == '\t')) {
            value_end--;
        }
        size_t value_len = value_end - value_start;
        struct http_header *header = &req->headers[req->nheaders];
        if (name_len >= sizeof(header->name) || value_len >= sizeof(header->value)) {
            return HTTP_HEADER_TOO_LARGE;
        }
        memcpy(header->name, buf + pos, name_len);
        header->name[name_len] = '\0';
        memcpy(header->value, buf + value_start, value_len);
        header->value[value_len] = '\0';
        req->nheaders++;
        pos = end + 2;
    }
    if (req->header_len == 0) {
        return HTTP_BAD_REQUEST;
    }
    if (strcmp(req->version, "HTTP/1.1") == 0 &&
        http_header_get(req, "Host") == NULL) {
        return HTTP_BAD_REQUEST;
    }
    if (http_header_get(req, "Transfer-Encoding") != NULL) {
        return HTTP_NOT_IMPLEMENTED;
    }
    const char *content_length = http_header_get(req, "Content-Length");
    if (content_length != NULL) {
        for (const char *digit = content_length; *digit != '\0'; digit++) {
            if (*digit < '0' || *digit > '9') {
                return HTTP_BAD_REQUEST;
            }
        }
        errno = 0;
        char *number_end = NULL;
        long body_length = strtol(content_length, &number_end, 10);
        if (errno == ERANGE || number_end == content_length ||
            *number_end != '\0' || body_length < 0) {
            return HTTP_BAD_REQUEST;
        }
        if (body_length > HTTP_MAX_BODY) {
            return HTTP_PAYLOAD_TOO_LARGE;
        }
        req->content_length = body_length;
    }
    return HTTP_OK;
}
