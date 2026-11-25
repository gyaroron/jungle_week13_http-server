#include "log.h"

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

#define LOG_LINE_CAP 4096

static pthread_mutex_t g_log_mutex = PTHREAD_MUTEX_INITIALIZER;

// 먼저 한 줄을 만들고, mutex를 잡은 동안 stderr에 끝까지 씁니다.
void server_log(const char *format, ...) {
    char line[LOG_LINE_CAP];
    va_list args;
    va_start(args, format);
    int length = vsnprintf(line, sizeof(line), format, args);
    va_end(args);

    if (length < 0) return;
    size_t remaining = (size_t)length;
    if (remaining >= sizeof(line)) remaining = sizeof(line) - 1;

    int lock_result = pthread_mutex_lock(&g_log_mutex);
    if (lock_result != 0) return;

    size_t written = 0;
    while (written < remaining) {
        ssize_t count = write(STDERR_FILENO, line + written, remaining - written);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) break;
        written += (size_t)count;
    }

    pthread_mutex_unlock(&g_log_mutex);
}

// 오류 문자열 함수의 공유 버퍼를 쓰지 않고 오류 번호를 기록합니다.
void server_log_errno(const char *operation, int error_number) {
    server_log("%s failed (errno=%d)\n", operation, error_number);
}
