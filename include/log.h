#ifndef LOG_H
#define LOG_H

// 여러 스레드가 함께 쓰는 서버 기록을 한 줄씩 안전하게 출력합니다.
void server_log(const char *format, ...);
void server_log_errno(const char *operation, int error_number);

#endif
