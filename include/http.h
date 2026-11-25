#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

#define HTTP_MAX_BODY 1048576

// 요청 해석 결과를 호출한 프로그램에 알려 주는 값입니다.
enum http_result {
    HTTP_OK,
    HTTP_INCOMPLETE,
    HTTP_BAD_REQUEST,
    HTTP_URI_TOO_LONG,
    HTTP_HEADER_TOO_LARGE,
    HTTP_NOT_IMPLEMENTED,
    HTTP_VERSION_NOT_SUPPORTED,
    HTTP_PAYLOAD_TOO_LARGE
};

// 해석한 요청 줄과 이후 헤더 정보를 담는 구조체입니다.
struct http_header {
    char name[64];
    char value[1024];
};

struct http_request {
    char method[8];
    char target[2048];
    char version[16];
    struct http_header headers[32];
    int nheaders;
    long content_length;
    int keep_alive;
    size_t header_len;
};

// 입력 버퍼를 수정하지 않고 HTTP 요청을 해석합니다.
enum http_result http_parse(const char *buf, size_t len, struct http_request *req);
const char *http_header_get(const struct http_request *req, const char *name);
const char *http_result_name(enum http_result result);

#endif
