#include "log.h"

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define LOG_LINE_CAP 4096
#define LOG_QUEUE_CAP (256 * 1024)

// 요청 스레드는 이 원형 버퍼에 줄을 넣기만 하고, 출력은 기록 전용 스레드가 맡습니다.
// stderr가 막혀도 요청 스레드는 기다리지 않습니다. 버퍼가 차면 그 줄을 버리고 수를 셉니다.
static pthread_mutex_t g_log_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_log_ready = PTHREAD_COND_INITIALIZER;
static pthread_cond_t g_log_drained = PTHREAD_COND_INITIALIZER;
static char g_queue[LOG_QUEUE_CAP];
static size_t g_head;
static size_t g_len;
static unsigned long g_dropped;
static int g_writing;
static int g_writer_ready;
static pthread_once_t g_writer_once = PTHREAD_ONCE_INIT;

// stderr에 끝까지 씁니다. 이 함수만 차단될 수 있고, 기록 스레드 안에서만 부릅니다.
static void write_stderr(const char *data, size_t length) {
    size_t written = 0;
    while (written < length) {
        ssize_t count = write(STDERR_FILENO, data + written, length - written);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return;
        written += (size_t)count;
    }
}

static void *log_writer(void *argument) {
    (void)argument;
    static char chunk[LOG_QUEUE_CAP];
    for (;;) {
        pthread_mutex_lock(&g_log_mutex);
        while (g_len == 0) {
            g_writing = 0;
            pthread_cond_broadcast(&g_log_drained);
            pthread_cond_wait(&g_log_ready, &g_log_mutex);
        }
        size_t take = g_len;
        size_t first = LOG_QUEUE_CAP - g_head;
        if (first > take) first = take;
        memcpy(chunk, g_queue + g_head, first);
        memcpy(chunk + first, g_queue, take - first);
        g_head = (g_head + take) % LOG_QUEUE_CAP;
        g_len = 0;
        unsigned long dropped = g_dropped;
        g_dropped = 0;
        g_writing = 1;
        pthread_mutex_unlock(&g_log_mutex);

        if (dropped > 0) {
            char note[96];
            int n = snprintf(note, sizeof(note), "log: dropped %lu lines while output was blocked\n", dropped);
            if (n > 0) write_stderr(note, (size_t)n < sizeof(note) ? (size_t)n : sizeof(note) - 1);
        }
        write_stderr(chunk, take);
    }
    return NULL;
}

static void start_writer(void) {
    pthread_attr_t attr;
    if (pthread_attr_init(&attr) != 0) return;
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_t thread;
    if (pthread_create(&thread, &attr, log_writer, NULL) == 0) g_writer_ready = 1;
    pthread_attr_destroy(&attr);
}

// 먼저 한 줄을 만들고, 짧게 mutex를 잡은 동안 대기열에 복사만 합니다.
void server_log(const char *format, ...) {
    char line[LOG_LINE_CAP];
    va_list args;
    va_start(args, format);
    int length = vsnprintf(line, sizeof(line), format, args);
    va_end(args);

    if (length < 0) return;
    size_t remaining = (size_t)length;
    if (remaining >= sizeof(line)) remaining = sizeof(line) - 1;

    pthread_once(&g_writer_once, start_writer);
    if (pthread_mutex_lock(&g_log_mutex) != 0) return;
    if (!g_writer_ready) {
        // 기록 스레드를 만들지 못한 경우에만 예전처럼 직접 씁니다.
        write_stderr(line, remaining);
    } else if (LOG_QUEUE_CAP - g_len >= remaining) {
        size_t tail = (g_head + g_len) % LOG_QUEUE_CAP;
        size_t first = LOG_QUEUE_CAP - tail;
        if (first > remaining) first = remaining;
        memcpy(g_queue + tail, line, first);
        memcpy(g_queue, line + first, remaining - first);
        g_len += remaining;
        pthread_cond_signal(&g_log_ready);
    } else {
        g_dropped++;
    }
    pthread_mutex_unlock(&g_log_mutex);
}

// 오류 문자열 함수의 공유 버퍼를 쓰지 않고 오류 번호를 기록합니다.
void server_log_errno(const char *operation, int error_number) {
    server_log("%s failed (errno=%d)\n", operation, error_number);
}

void server_log_flush(int timeout_ms) {
    if (pthread_mutex_lock(&g_log_mutex) != 0) return;
    if (g_writer_ready) {
        // pthread_cond_timedwait는 실제 시각(realtime) 기준 절대 시각을 받습니다.
        struct timeval now;
        gettimeofday(&now, NULL);
        struct timespec deadline;
        long long ns = (long long)now.tv_usec * 1000 + (long long)(timeout_ms % 1000) * 1000000;
        deadline.tv_sec = now.tv_sec + timeout_ms / 1000 + (time_t)(ns / 1000000000);
        deadline.tv_nsec = (long)(ns % 1000000000);
        while (g_len > 0 || g_writing) {
            if (pthread_cond_timedwait(&g_log_drained, &g_log_mutex, &deadline) == ETIMEDOUT) break;
        }
    }
    pthread_mutex_unlock(&g_log_mutex);
}
