// event loop 모드의 conn API를 socketpair로 검사합니다.
// loop 쪽 conn_read_available()과 worker 쪽 conn_resume()이 나눠 처리해도
// Blocking 모드(conn_handle)와 같은 응답 · keep-alive · pipelining 결과가 나와야 합니다.
#include "config.h"
#include "conn.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static struct config test_config(void) {
    struct config cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.host = "127.0.0.1";
    cfg.root = "./www";
    cfg.workers = 1;
    cfg.timeout_sec = 2;
    return cfg;
}

static void send_text(int fd, const char *text) {
    size_t len = strlen(text);
    assert(send(fd, text, len, 0) == (ssize_t)len);
}

// 응답 바이트를 읽어 둡니다. socketpair 송신 버퍼에 이미 다 들어와 있으므로 한 번에 충분합니다.
static size_t read_reply(int fd, char *buf, size_t cap) {
    ssize_t n = recv(fd, buf, cap - 1, 0);
    assert(n > 0);
    buf[n] = '\0';
    return (size_t)n;
}

static int count_status_lines(const char *text, const char *status) {
    int count = 0;
    for (const char *p = text; (p = strstr(p, status)) != NULL; p += strlen(status)) count++;
    return count;
}

static void test_partial_header_then_keepalive(const struct config *cfg) {
    int pair[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    struct conn *c = conn_create(pair[0], "127.0.0.1", cfg);
    assert(c != NULL);
    assert(conn_get_fd(c) == pair[0]);
    assert(conn_deadline_ms(c) > conn_now_ms());

    // 헤더 앞부분만 오면 loop가 계속 기다려야 합니다.
    assert(conn_read_available(c) == CONN_READ_MORE);  // 아직 아무 바이트도 없음
    send_text(pair[1], "GET / HTTP/1.1\r\nHo");
    assert(conn_read_available(c) == CONN_READ_MORE);
    assert(conn_has_partial_request(c));

    // 헤더 끝이 오면 worker에 넘길 준비가 됩니다.
    send_text(pair[1], "st: localhost\r\n\r\n");
    assert(conn_read_available(c) == CONN_READ_READY);

    // HTTP/1.1 정적 응답은 keep-alive이므로 처리 뒤 연결을 loop에 돌려줍니다.
    assert(conn_resume(c) == CONN_RUN_PARKED);
    char reply[16384];
    read_reply(pair[1], reply, sizeof(reply));
    assert(strncmp(reply, "HTTP/1.1 200 OK", 15) == 0);
    assert(strstr(reply, "Connection: keep-alive") != NULL);
    assert(!conn_has_partial_request(c));

    // 두 요청을 한 번에 보내면(pipelining) worker가 둘 다 처리하고, close 요청 뒤 연결을 끝냅니다.
    send_text(pair[1], "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"
                       "GET /no-such-file HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");
    assert(conn_read_available(c) == CONN_READ_READY);
    assert(conn_resume(c) == CONN_RUN_CLOSED);
    conn_destroy(c);

    char all[32768];
    size_t used = 0;
    ssize_t n;
    while ((n = recv(pair[1], all + used, sizeof(all) - 1 - used, 0)) > 0) used += (size_t)n;
    all[used] = '\0';
    assert(count_status_lines(all, "HTTP/1.1 200 OK") == 1);
    assert(count_status_lines(all, "HTTP/1.1 404 Not Found") == 1);
    close(pair[1]);
}

static void test_peer_close(const struct config *cfg) {
    int pair[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    struct conn *c = conn_create(pair[0], "127.0.0.1", cfg);
    assert(c != NULL);
    send_text(pair[1], "GET / HTTP/1.1\r\n");
    assert(conn_read_available(c) == CONN_READ_MORE);
    close(pair[1]);
    // 상대가 닫으면 loop가 연결을 정리해야 합니다.
    assert(conn_read_available(c) == CONN_READ_CLOSE);
    conn_destroy(c);
}

static void test_bad_request_closes(const struct config *cfg) {
    int pair[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    struct conn *c = conn_create(pair[0], "127.0.0.1", cfg);
    assert(c != NULL);
    send_text(pair[1], "GARBAGE\r\n\r\n");
    assert(conn_read_available(c) == CONN_READ_READY);
    assert(conn_resume(c) == CONN_RUN_CLOSED);
    conn_destroy(c);
    char reply[4096];
    read_reply(pair[1], reply, sizeof(reply));
    assert(strncmp(reply, "HTTP/1.1 400 Bad Request", 24) == 0);
    close(pair[1]);
}

int main(void) {
    struct config cfg = test_config();
    test_partial_header_then_keepalive(&cfg);
    test_peer_close(&cfg);
    test_bad_request_closes(&cfg);
    puts("all event-mode conn tests passed");
    return 0;
}
