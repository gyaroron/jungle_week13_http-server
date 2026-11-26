#ifndef CONN_H
#define CONN_H

#include "config.h"

// 이 헤더를 여러 번 포함해도 같은 선언이 중복 처리되지 않게 막습니다.

// 연결 하나에서 요청을 읽고 응답을 보낸 뒤 전달받은 소켓 번호를 닫습니다.
// thread pool · 연결별 thread · 순차 모델이 쓰는 Blocking 처리 경로입니다.
void conn_handle(int fd, const char *client_ip, const struct config *cfg);

// ---- event loop 모드 (-e) -------------------------------------------------
// 연결 하나의 상태를 힙에 두고, loop thread와 worker thread가 번갈아 이어 처리합니다.
struct conn;

enum conn_read_result {
    CONN_READ_MORE,   // 헤더 끝(\r\n\r\n)이 아직 없습니다. loop가 다음 바이트를 기다립니다.
    CONN_READ_READY,  // 헤더 끝이 도착했습니다. worker에 넘겨 요청을 처리합니다.
    CONN_READ_CLOSE   // 상대가 닫았거나 오류이거나 헤더가 버퍼를 넘었습니다.
};

enum conn_run_result {
    CONN_RUN_PARKED,  // keep-alive로 다음 요청을 기다립니다. 연결을 loop에 돌려줍니다.
    CONN_RUN_CLOSED   // 처리가 끝났습니다. 호출자가 conn_destroy()로 닫습니다.
};

struct conn *conn_create(int fd, const char *client_ip, const struct config *cfg);
int conn_get_fd(const struct conn *c);
// 요청 하나를 받아야 하는 monotonic clock 기준 마감 시각(ms)입니다.
long long conn_deadline_ms(const struct conn *c);
// 받다 만 요청 바이트가 버퍼에 있으면 1입니다. 마감 때 408을 보낼지 정합니다.
int conn_has_partial_request(const struct conn *c);
// loop thread: 소켓에서 지금 읽을 수 있는 바이트만 기다리지 않고 읽습니다.
enum conn_read_result conn_read_available(struct conn *c);
// worker thread: 받은 요청을 처리하고, 다음 요청이 아직 없으면 PARKED를 돌려줍니다.
enum conn_run_result conn_resume(struct conn *c);
// 소켓을 닫고 연결 상태를 해제합니다.
void conn_destroy(struct conn *c);
long long conn_now_ms(void);

#endif
