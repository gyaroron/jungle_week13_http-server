#ifndef STATIC_H
#define STATIC_H

#include "config.h"
#include "http.h"

// 0은 파일 응답 완료, 1은 오류 응답 완료, -1은 전송 실패입니다.
int static_serve(int fd, const struct config *cfg, const struct http_request *req);

#endif
