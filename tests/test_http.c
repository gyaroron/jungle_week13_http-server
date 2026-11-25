#include "http.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static enum http_result parse(const char *text, struct http_request *req) {
    return http_parse(text, strlen(text), req);
}

static void test_valid_get_and_trimmed_header(void) {
    const char *text = "GET /a HTTP/1.1\r\nHost: example.test\r\nX-Test:  \thi \t \r\n\r\n";
    struct http_request req;
    assert(parse(text, &req) == HTTP_OK);
    assert(strcmp(req.method, "GET") == 0);
    assert(strcmp(req.target, "/a") == 0);
    assert(strcmp(http_header_get(&req, "x-tEsT"), "hi") == 0);
}

static void test_post_content_length(void) {
    const char *text = "POST /echo HTTP/1.1\r\nHost: example.test\r\nContent-Length: 7\r\n\r\na=1&b=2";
    struct http_request req;
    assert(parse(text, &req) == HTTP_OK);
    assert(req.content_length == 7);
    assert(req.header_len == strlen(text) - 7);
}

static void test_incomplete_and_bad_request_line(void) {
    struct http_request req;
    const char *incomplete = "GET / HTTP/1.1\r\nHost: x\r\n";
    assert(http_parse(incomplete, strlen(incomplete), &req) == HTTP_INCOMPLETE);
    assert(parse("GET /only-one-space\r\n\r\n", &req) == HTTP_BAD_REQUEST);
}

static void test_unsupported_method_and_version(void) {
    struct http_request req;
    assert(parse("PUT / HTTP/1.1\r\nHost: x\r\n\r\n", &req) == HTTP_NOT_IMPLEMENTED);
    assert(parse("GET / HTTP/9.9\r\nHost: x\r\n\r\n", &req) == HTTP_VERSION_NOT_SUPPORTED);
}

static void test_long_target_and_too_many_headers(void) {
    struct http_request req;
    char *long_target = malloc(2100);
    assert(long_target != NULL);
    const char *prefix = "GET /";
    size_t pos = strlen(prefix);
    memcpy(long_target, prefix, pos);
    memset(long_target + pos, 'a', 2048);
    pos += 2048;
    const char *suffix = " HTTP/1.1\r\nHost: x\r\n\r\n";
    size_t suffix_len = strlen(suffix);
    memcpy(long_target + pos, suffix, suffix_len);
    pos += suffix_len;
    assert(http_parse(long_target, pos, &req) == HTTP_URI_TOO_LONG);
    free(long_target);

    char headers[1200] = "GET / HTTP/1.1\r\nHost: x\r\n";
    for (int i = 0; i < 32; i++) {
        strcat(headers, "X: y\r\n");
    }
    strcat(headers, "\r\n");
    assert(parse(headers, &req) == HTTP_HEADER_TOO_LARGE);
}

static void test_bad_header_and_content_length(void) {
    struct http_request req;
    assert(parse("GET / HTTP/1.1\r\nHost x\r\n\r\n", &req) == HTTP_BAD_REQUEST);
    assert(parse("POST / HTTP/1.1\r\nHost: x\r\nContent-Length: abc\r\n\r\n", &req) == HTTP_BAD_REQUEST);
    assert(parse("POST / HTTP/1.1\r\nHost: x\r\nContent-Length: -1\r\n\r\n", &req) == HTTP_BAD_REQUEST);
    assert(parse("POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 2000000\r\n\r\n", &req) == HTTP_PAYLOAD_TOO_LARGE);
}

static void test_host_and_transfer_encoding(void) {
    struct http_request req;
    assert(parse("GET / HTTP/1.1\r\nX: y\r\n\r\n", &req) == HTTP_BAD_REQUEST);
    assert(parse("GET / HTTP/1.1\r\nhOsT: example.test\r\n\r\n", &req) == HTTP_OK);
    assert(parse("POST / HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\n", &req) == HTTP_NOT_IMPLEMENTED);
}

int main(void) {
    test_valid_get_and_trimmed_header();
    test_post_content_length();
    test_incomplete_and_bad_request_line();
    test_unsupported_method_and_version();
    test_long_target_and_too_many_headers();
    test_bad_header_and_content_length();
    test_host_and_transfer_encoding();
    puts("all HTTP parser tests passed");
    return 0;
}
