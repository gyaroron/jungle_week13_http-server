#include "evloop.h"

#include "conn.h"
#include "log.h"
#include "net.h"
#include "response.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#if defined(__linux__) && !defined(EVLOOP_USE_KQUEUE)
#define EVLOOP_USE_EPOLL 1
#endif

#ifdef EVLOOP_USE_EPOLL
#include <sys/epoll.h>
#else
#include <sys/event.h>
#include <sys/time.h>
#endif

#define EV_WORK_CAP 1024     // 헤더가 다 온 연결을 worker에 넘기는 대기열 상한
#define EV_TICK_MS 100       // 마감 시각 검사와 종료 신호 확인 주기
#define EV_BATCH 128         // 한 번에 꺼내는 이벤트 수

// loop가 맡은 연결 하나입니다. 이벤트의 사용자 데이터(udata)로 이 주소를 돌려받습니다.
struct ev_conn {
    struct conn *conn;
    struct ev_conn *prev;
    struct ev_conn *next;
    int armed_before;        // epoll: ADD를 한 번 했으면 이후에는 MOD로 다시 켭니다.
};

// worker에 넘긴 연결과 worker가 돌려준 연결을 담는 단순 연결 리스트입니다.
struct job {
    struct ev_conn *ec;
    struct job *next;
};

struct evloop {
    int poll_fd;             // kqueue 또는 epoll 번호
    int wake_rd;             // worker가 연결을 돌려줄 때 loop를 깨우는 pipe
    int wake_wr;
    int listen_fd;
    const struct config *cfg;

    struct ev_conn *idle_head;   // loop가 감시 중인 연결 목록 (마감 시각 검사용)

    pthread_mutex_t work_mutex;
    pthread_cond_t work_ready;
    struct job *work_head, *work_tail;
    int work_count;
    int stopping;

    pthread_mutex_t back_mutex;
    struct job *back_head;       // worker가 keep-alive로 돌려준 연결

    pthread_t *threads;
    int thread_count;
};

static struct ev_conn *const LISTEN_TAG = (struct ev_conn *)1;
static struct ev_conn *const WAKE_TAG = (struct ev_conn *)2;
static struct ev_conn *const SIGNAL_TAG = (struct ev_conn *)3;

// ---- kqueue / epoll 얇은 감싸기 ---------------------------------------------

static int poll_create(void) {
#ifdef EVLOOP_USE_EPOLL
    return epoll_create1(EPOLL_CLOEXEC);
#else
    int fd = kqueue();
    if (fd >= 0) (void)fcntl(fd, F_SETFD, FD_CLOEXEC);
    return fd;
#endif
}

// 계속 감시할 번호(listen socket, wake pipe)를 감시 목록에 넣습니다.
static int poll_add_persistent(struct evloop *ev, int fd, void *tag) {
#ifdef EVLOOP_USE_EPOLL
    struct epoll_event e;
    memset(&e, 0, sizeof(e));
    e.events = EPOLLIN;
    e.data.ptr = tag;
    return epoll_ctl(ev->poll_fd, EPOLL_CTL_ADD, fd, &e);
#else
    struct kevent k;
    EV_SET(&k, fd, EVFILT_READ, EV_ADD, 0, 0, tag);
    return kevent(ev->poll_fd, &k, 1, NULL, 0, NULL);
#endif
}

// SIGINT/SIGTERM을 kqueue 이벤트로도 받습니다. 시그널 핸들러가 늦게 실행되는 환경
// (예: macOS ThreadSanitizer는 가로챈 함수에 들어갈 때까지 핸들러를 미룹니다)에서도
// loop가 종료 요청을 놓치지 않게 합니다. epoll 경로는 기존 핸들러의 g_stop만 봅니다.
static void poll_add_stop_signals(struct evloop *ev) {
#ifndef EVLOOP_USE_EPOLL
    struct kevent k[2];
    EV_SET(&k[0], SIGINT, EVFILT_SIGNAL, EV_ADD, 0, 0, SIGNAL_TAG);
    EV_SET(&k[1], SIGTERM, EVFILT_SIGNAL, EV_ADD, 0, 0, SIGNAL_TAG);
    if (kevent(ev->poll_fd, k, 2, NULL, 0, NULL) < 0) server_log_errno("kevent signal", errno);
#else
    (void)ev;
#endif
}

// 연결 하나를 "읽을 수 있으면 한 번만 알림"으로 켭니다. 알린 뒤에는 자동으로 꺼지므로
// worker가 처리하는 동안 loop가 같은 연결을 다시 건드리지 않습니다.
static int poll_arm_oneshot(struct evloop *ev, struct ev_conn *ec) {
    int fd = conn_get_fd(ec->conn);
#ifdef EVLOOP_USE_EPOLL
    struct epoll_event e;
    memset(&e, 0, sizeof(e));
    e.events = EPOLLIN | EPOLLRDHUP | EPOLLONESHOT;
    e.data.ptr = ec;
    int op = ec->armed_before ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    ec->armed_before = 1;
    return epoll_ctl(ev->poll_fd, op, fd, &e);
#else
    struct kevent k;
    EV_SET(&k, fd, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, ec);
    return kevent(ev->poll_fd, &k, 1, NULL, 0, NULL);
#endif
}

// 준비된 이벤트의 udata 목록을 돌려줍니다. 시간 초과면 0입니다.
static int poll_wait(struct evloop *ev, void **tags, int cap, int timeout_ms) {
#ifdef EVLOOP_USE_EPOLL
    struct epoll_event events[EV_BATCH];
    int n = epoll_wait(ev->poll_fd, events, cap < EV_BATCH ? cap : EV_BATCH, timeout_ms);
    for (int i = 0; i < n; i++) tags[i] = events[i].data.ptr;
    return n;
#else
    struct kevent events[EV_BATCH];
    struct timespec ts = {.tv_sec = timeout_ms / 1000, .tv_nsec = (long)(timeout_ms % 1000) * 1000000L};
    int n = kevent(ev->poll_fd, NULL, 0, events, cap < EV_BATCH ? cap : EV_BATCH, &ts);
    for (int i = 0; i < n; i++) tags[i] = events[i].udata;
    return n;
#endif
}

// ---- loop가 맡은 연결 목록 ----------------------------------------------------

static void idle_push(struct evloop *ev, struct ev_conn *ec) {
    ec->prev = NULL;
    ec->next = ev->idle_head;
    if (ev->idle_head != NULL) ev->idle_head->prev = ec;
    ev->idle_head = ec;
}

static void idle_remove(struct evloop *ev, struct ev_conn *ec) {
    if (ec->prev != NULL) ec->prev->next = ec->next;
    else ev->idle_head = ec->next;
    if (ec->next != NULL) ec->next->prev = ec->prev;
    ec->prev = ec->next = NULL;
}

// 닫기 전에 짧은 오류 응답을 한 번만 시도합니다. loop thread가 송신 버퍼를 기다리지 않게 합니다.
static void send_error_nowait(int fd, int status) {
    if (net_set_nonblock(fd) == 0) (void)resp_send_error(fd, status);
}

static void ev_conn_close(struct ev_conn *ec) {
    conn_destroy(ec->conn);
    free(ec);
}

// ---- worker thread -------------------------------------------------------------

static void wake_loop(struct evloop *ev) {
    char byte = 1;
    // pipe가 가득 차 있어도 loop는 어차피 깨어날 예정이므로 실패를 무시합니다.
    ssize_t ignored = write(ev->wake_wr, &byte, 1);
    (void)ignored;
}

static void *worker_main(void *arg) {
    struct evloop *ev = arg;
    for (;;) {
        pthread_mutex_lock(&ev->work_mutex);
        while (ev->work_head == NULL && !ev->stopping) {
            pthread_cond_wait(&ev->work_ready, &ev->work_mutex);
        }
        if (ev->work_head == NULL) {
            pthread_mutex_unlock(&ev->work_mutex);
            return NULL;
        }
        struct job *job = ev->work_head;
        ev->work_head = job->next;
        if (ev->work_head == NULL) ev->work_tail = NULL;
        ev->work_count--;
        pthread_mutex_unlock(&ev->work_mutex);

        struct ev_conn *ec = job->ec;
        if (conn_resume(ec->conn) == CONN_RUN_PARKED) {
            // keep-alive: 다음 요청을 기다리는 일은 loop에 돌려줍니다. worker는 바로 다음 일을 받습니다.
            pthread_mutex_lock(&ev->back_mutex);
            job->next = ev->back_head;
            ev->back_head = job;
            pthread_mutex_unlock(&ev->back_mutex);
            wake_loop(ev);
        } else {
            ev_conn_close(ec);
            free(job);
        }
    }
}

// 헤더가 다 온 연결을 worker 대기열에 넣습니다. 대기열이 가득 차면 503으로 닫습니다.
static void dispatch(struct evloop *ev, struct ev_conn *ec) {
    struct job *job = malloc(sizeof(*job));
    pthread_mutex_lock(&ev->work_mutex);
    if (job == NULL || ev->work_count >= EV_WORK_CAP) {
        pthread_mutex_unlock(&ev->work_mutex);
        server_log("fd %d: event worker queue full, sending 503\n", conn_get_fd(ec->conn));
        send_error_nowait(conn_get_fd(ec->conn), 503);
        free(job);
        ev_conn_close(ec);
        return;
    }
    job->ec = ec;
    job->next = NULL;
    if (ev->work_tail != NULL) ev->work_tail->next = job;
    else ev->work_head = job;
    ev->work_tail = job;
    ev->work_count++;
    pthread_cond_signal(&ev->work_ready);
    pthread_mutex_unlock(&ev->work_mutex);
}

// ---- loop thread가 처리하는 이벤트 ---------------------------------------------------

static void watch(struct evloop *ev, struct ev_conn *ec) {
    if (poll_arm_oneshot(ev, ec) < 0) {
        server_log_errno("event arm", errno);
        ev_conn_close(ec);
        return;
    }
    idle_push(ev, ec);
}

static void accept_ready(struct evloop *ev) {
    for (;;) {
        struct sockaddr_in peer;
        socklen_t peer_len = sizeof(peer);
        int fd = accept(ev->listen_fd, (struct sockaddr *)&peer, &peer_len);
        if (fd < 0) {
            if (errno == EINTR) continue;
            if (errno != EAGAIN && errno != EWOULDBLOCK) server_log_errno("accept", errno);
            return;
        }
        if (net_prepare_client(fd, ev->cfg->timeout_sec) < 0) {
            server_log_errno("prepare client socket", errno);
            close(fd);
            continue;
        }
        char ip[INET_ADDRSTRLEN] = "unknown";
        (void)inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof(ip));
        server_log("accepted fd %d from %s:%d (event loop)\n", fd, ip, ntohs(peer.sin_port));

        struct ev_conn *ec = calloc(1, sizeof(*ec));
        struct conn *c = ec != NULL ? conn_create(fd, ip, ev->cfg) : NULL;
        if (c == NULL) {
            server_log_errno("allocate connection", errno);
            free(ec);
            close(fd);
            continue;
        }
        ec->conn = c;
        watch(ev, ec);
    }
}

static void conn_ready(struct evloop *ev, struct ev_conn *ec) {
    idle_remove(ev, ec);
    switch (conn_read_available(ec->conn)) {
        case CONN_READ_READY:
            dispatch(ev, ec);
            break;
        case CONN_READ_MORE:
            watch(ev, ec);
            break;
        case CONN_READ_CLOSE:
            ev_conn_close(ec);
            break;
    }
}

// worker가 keep-alive로 돌려준 연결을 다시 감시합니다.
static void take_back(struct evloop *ev) {
    char drain[256];
    while (read(ev->wake_rd, drain, sizeof(drain)) > 0) {
    }
    pthread_mutex_lock(&ev->back_mutex);
    struct job *list = ev->back_head;
    ev->back_head = NULL;
    pthread_mutex_unlock(&ev->back_mutex);
    while (list != NULL) {
        struct job *next = list->next;
        watch(ev, list->ec);
        free(list);
        list = next;
    }
}

// 마감 시각이 지난 연결을 닫습니다. 받다 만 요청이 있으면 Blocking 모드처럼 408을 보냅니다.
static void expire(struct evloop *ev) {
    long long now = conn_now_ms();
    struct ev_conn *ec = ev->idle_head;
    while (ec != NULL) {
        struct ev_conn *next = ec->next;
        if (conn_deadline_ms(ec->conn) <= now) {
            int fd = conn_get_fd(ec->conn);
            if (conn_has_partial_request(ec->conn)) send_error_nowait(fd, 408);
            server_log("fd %d: request read timed out (event loop)\n", fd);
            idle_remove(ev, ec);
            ev_conn_close(ec);  // close()가 kqueue/epoll 감시 항목도 함께 지웁니다.
        }
        ec = next;
    }
}

// ---- 시작과 정리 -------------------------------------------------------------------

static void shutdown_workers(struct evloop *ev) {
    pthread_mutex_lock(&ev->work_mutex);
    ev->stopping = 1;
    // 아직 시작하지 않은 요청은 thread pool 모드처럼 503으로 돌려보냅니다.
    struct job *pending = ev->work_head;
    ev->work_head = ev->work_tail = NULL;
    ev->work_count = 0;
    pthread_cond_broadcast(&ev->work_ready);
    pthread_mutex_unlock(&ev->work_mutex);
    int rejected = 0;
    while (pending != NULL) {
        struct job *next = pending->next;
        send_error_nowait(conn_get_fd(pending->ec->conn), 503);
        ev_conn_close(pending->ec);
        free(pending);
        pending = next;
        rejected++;
    }
    if (rejected > 0) server_log("shutdown: rejected %d queued requests with 503\n", rejected);
    for (int i = 0; i < ev->thread_count; i++) pthread_join(ev->threads[i], NULL);
}

static void close_all(struct evloop *ev) {
    while (ev->idle_head != NULL) {
        struct ev_conn *ec = ev->idle_head;
        idle_remove(ev, ec);
        ev_conn_close(ec);
    }
    struct job *list = ev->back_head;
    ev->back_head = NULL;
    while (list != NULL) {
        struct job *next = list->next;
        ev_conn_close(list->ec);
        free(list);
        list = next;
    }
}

int evloop_run(int listen_fd, const struct config *cfg, volatile sig_atomic_t *stop) {
    struct evloop ev;
    memset(&ev, 0, sizeof(ev));
    ev.listen_fd = listen_fd;
    ev.cfg = cfg;
    ev.poll_fd = -1;
    ev.wake_rd = ev.wake_wr = -1;

    int pipe_fds[2];
    ev.poll_fd = poll_create();
    if (ev.poll_fd < 0 || pipe(pipe_fds) < 0) {
        server_log_errno("event loop setup", errno);
        if (ev.poll_fd >= 0) close(ev.poll_fd);
        return -1;
    }
    ev.wake_rd = pipe_fds[0];
    ev.wake_wr = pipe_fds[1];
    for (int i = 0; i < 2; i++) {
        (void)net_set_nonblock(pipe_fds[i]);
        (void)fcntl(pipe_fds[i], F_SETFD, FD_CLOEXEC);
    }
    pthread_mutex_init(&ev.work_mutex, NULL);
    pthread_cond_init(&ev.work_ready, NULL);
    pthread_mutex_init(&ev.back_mutex, NULL);

    int result = 0;
    if (poll_add_persistent(&ev, listen_fd, LISTEN_TAG) < 0 ||
        poll_add_persistent(&ev, ev.wake_rd, WAKE_TAG) < 0) {
        server_log_errno("event register", errno);
        result = -1;
        goto cleanup;
    }

    poll_add_stop_signals(&ev);

    ev.threads = calloc((size_t)cfg->workers, sizeof(*ev.threads));
    if (ev.threads == NULL) {
        result = -1;
        goto cleanup;
    }
    for (int i = 0; i < cfg->workers; i++) {
        if (pthread_create(&ev.threads[i], NULL, worker_main, &ev) != 0) break;
        ev.thread_count++;
    }
    if (ev.thread_count == 0) {
        server_log("event loop: no worker thread started\n");
        result = -1;
        goto cleanup;
    }
#ifdef EVLOOP_USE_EPOLL
    server_log("event loop: epoll, %d workers\n", ev.thread_count);
#else
    server_log("event loop: kqueue, %d workers\n", ev.thread_count);
#endif

    int signalled = 0;
    while (!*stop && !signalled) {
        void *tags[EV_BATCH];
        int n = poll_wait(&ev, tags, EV_BATCH, EV_TICK_MS);
        if (n < 0) {
            if (errno == EINTR) continue;
            server_log_errno("event wait", errno);
            continue;
        }
        for (int i = 0; i < n; i++) {
            if (tags[i] == LISTEN_TAG) accept_ready(&ev);
            else if (tags[i] == WAKE_TAG) take_back(&ev);
            else if (tags[i] == SIGNAL_TAG) signalled = 1;
            else conn_ready(&ev, (struct ev_conn *)tags[i]);
        }
        expire(&ev);
    }

    shutdown_workers(&ev);

cleanup:
    close_all(&ev);
    free(ev.threads);
    pthread_mutex_destroy(&ev.back_mutex);
    pthread_cond_destroy(&ev.work_ready);
    pthread_mutex_destroy(&ev.work_mutex);
    close(ev.wake_rd);
    close(ev.wake_wr);
    close(ev.poll_fd);
    return result;
}
