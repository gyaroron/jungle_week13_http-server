#ifndef EVLOOP_H
#define EVLOOP_H

#include <signal.h>

#include "config.h"

// listen socket과 모든 대기 연결을 kqueue(macOS/BSD) 또는 epoll(Linux) 하나로 감시합니다.
// 헤더가 다 도착한 연결만 cfg->workers개의 worker thread에 넘기고,
// keep-alive로 다음 요청을 기다리는 연결은 다시 loop가 맡습니다.
// *stop이 0이 아니게 되면 새 연결을 받지 않고 정리한 뒤 0을 돌려줍니다. 시작 실패는 -1입니다.
int evloop_run(int listen_fd, const struct config *cfg, volatile sig_atomic_t *stop);

#endif
