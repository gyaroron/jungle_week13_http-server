#ifndef NET_H
#define NET_H

#include <stddef.h>

// 이 헤더를 여러 번 포함해도 같은 선언이 중복 처리되지 않게 막습니다.

// host와 port에 바인딩해 연결 요청을 기다리는 소켓을 만듭니다.
// 성공하면 듣는 소켓의 번호를, 실패하면 -1을 반환합니다.
int net_listen(const char *host, int port, int backlog);
int net_connect(const char *host, const char *port, int timeout_ms);
int net_send_all(int fd, const void *buf, size_t len);
int net_set_timeouts(int fd, int seconds);
int net_set_nonblock(int fd);
int net_set_blocking(int fd);
int net_wait(int fd, short events, int timeout_ms);
// accept()로 받은 연결 소켓을 Blocking + 송수신 제한 시간 + (macOS) SO_NOSIGPIPE로 맞춥니다.
int net_prepare_client(int fd, int timeout_sec);

#endif
