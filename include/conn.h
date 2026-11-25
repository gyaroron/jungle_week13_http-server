#ifndef CONN_H
#define CONN_H

#include "config.h"

// 이 헤더를 여러 번 포함해도 같은 선언이 중복 처리되지 않게 막습니다.

// 연결 하나에서 요청을 읽고 응답을 보낸 뒤 전달받은 소켓 번호를 닫습니다.
void conn_handle(int fd, const char *client_ip, const struct config *cfg);

#endif
