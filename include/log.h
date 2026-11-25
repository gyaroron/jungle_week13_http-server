#ifndef LOG_H
#define LOG_H

// 여러 스레드가 함께 쓰는 서버 기록을 한 줄씩 대기열에 넣습니다.
// 실제 출력은 기록 전용 스레드가 하므로, stderr가 막혀도 호출한 스레드는 멈추지 않습니다.
void server_log(const char *format, ...);
void server_log_errno(const char *operation, int error_number);
// 종료 직전에 대기열이 비거나 제한 시간이 지날 때까지 기다립니다.
void server_log_flush(int timeout_ms);

#endif
