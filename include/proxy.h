#ifndef PROXY_H
#define PROXY_H

#include "config.h"
#include "http.h"
#include "cache.h"

#include <stddef.h>

enum proxy_result {
    PROXY_FAILED = -1,
    PROXY_HANDLED = 0,
    PROXY_NO_ROUTE = 1
};

// 일치하는 리버스 프록시 규칙이 없으면 PROXY_NO_ROUTE를 돌려줍니다.
int proxy_reverse(int client_fd, const struct http_request *req,
                  const char *body, size_t body_len, const struct config *cfg,
                  const char *client_ip);
int proxy_reverse_cached(int client_fd, const struct http_request *req,
                         const char *body, size_t body_len,
                         const struct config *cfg, const char *client_ip,
                         struct cache_capture *capture);

// absolute-form HTTP 요청을 해석해 포워드 프록시로 전달합니다.
int proxy_forward(int client_fd, const struct http_request *req,
                  const char *body, size_t body_len, int timeout_sec,
                  const char *client_ip);
int proxy_forward_cached(int client_fd, const struct http_request *req,
                         const char *body, size_t body_len, int timeout_sec,
                         const char *client_ip, struct cache_capture *capture);

#endif
