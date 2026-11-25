#include "pool.h"

#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>

struct pool {
    pthread_t *threads;
    int worker_count;
    int started_count;
    int *queue;
    size_t queue_cap;
    size_t head;
    size_t tail;
    size_t count;
    int stopping;
    void (*handler)(int fd);
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    int mutex_ready;
    int not_empty_ready;
    int not_full_ready;
};

// 일꾼은 큐가 비면 잠들고, 연결 하나를 꺼내 처리한 뒤 다시 기다립니다.
static void *pool_worker(void *argument) {
    struct pool *p = argument;

    for (;;) {
        pthread_mutex_lock(&p->mutex);
        while (p->count == 0 && !p->stopping) {
            pthread_cond_wait(&p->not_empty, &p->mutex);
        }

        if (p->count == 0 && p->stopping) {
            pthread_mutex_unlock(&p->mutex);
            return NULL;
        }

        int fd = p->queue[p->head];
        p->head = (p->head + 1) % p->queue_cap;
        p->count--;
        pthread_cond_signal(&p->not_full);
        pthread_mutex_unlock(&p->mutex);

        p->handler(fd);
    }
}

// mutex·조건 변수·대기열을 준비한 뒤 지정한 수만큼 일꾼을 시작합니다.
struct pool *pool_create(int workers, int queue_cap, void (*handler)(int fd)) {
    if (workers <= 0 || queue_cap <= 0 || handler == NULL) {
        errno = EINVAL;
        return NULL;
    }

    struct pool *p = calloc(1, sizeof(*p));
    if (p == NULL) return NULL;

    p->threads = calloc((size_t)workers, sizeof(*p->threads));
    p->queue = calloc((size_t)queue_cap, sizeof(*p->queue));
    if (p->threads == NULL || p->queue == NULL) goto fail;

    p->worker_count = workers;
    p->queue_cap = (size_t)queue_cap;
    p->handler = handler;

    int result = pthread_mutex_init(&p->mutex, NULL);
    if (result != 0) {
        errno = result;
        goto fail;
    }
    p->mutex_ready = 1;

    result = pthread_cond_init(&p->not_empty, NULL);
    if (result != 0) {
        errno = result;
        goto fail;
    }
    p->not_empty_ready = 1;

    result = pthread_cond_init(&p->not_full, NULL);
    if (result != 0) {
        errno = result;
        goto fail;
    }
    p->not_full_ready = 1;

    for (int i = 0; i < workers; i++) {
        result = pthread_create(&p->threads[i], NULL, pool_worker, p);
        if (result != 0) {
            errno = result;
            goto fail;
        }
        p->started_count++;
    }
    return p;

fail:
    pool_shutdown(p);
    return NULL;
}

// 연결 fd를 원형 대기열에 넣습니다. 큐가 가득 차 있으면 기다리지 않습니다.
int pool_submit(struct pool *p, int fd) {
    if (p == NULL) {
        errno = EINVAL;
        return -1;
    }

    int result = pthread_mutex_lock(&p->mutex);
    if (result != 0) {
        errno = result;
        return -1;
    }

    // 대기열이 가득 차면 기다리지 않고 바로 실패를 돌려줍니다.
    // 기다리면 accept() 반복 전체가 멈추고 종료 신호(g_stop)도 확인할 수 없습니다.
    if (p->count == p->queue_cap && !p->stopping) {
        pthread_mutex_unlock(&p->mutex);
        errno = EAGAIN;
        return -1;
    }

    if (p->stopping) {
        pthread_mutex_unlock(&p->mutex);
        errno = ECANCELED;
        return -1;
    }

    p->queue[p->tail] = fd;
    p->tail = (p->tail + 1) % p->queue_cap;
    p->count++;
    pthread_cond_signal(&p->not_empty);
    pthread_mutex_unlock(&p->mutex);
    return 0;
}

int pool_reject_queued(struct pool *p, void (*reject)(int fd)) {
    if (p == NULL || reject == NULL) return 0;
    int taken[256];
    int total = 0;
    for (;;) {
        // mutex 안에서는 fd 번호만 옮기고, 응답 전송은 mutex 밖에서 합니다.
        int n = 0;
        pthread_mutex_lock(&p->mutex);
        while (p->count > 0 && n < (int)(sizeof(taken) / sizeof(taken[0]))) {
            taken[n++] = p->queue[p->head];
            p->head = (p->head + 1) % p->queue_cap;
            p->count--;
        }
        pthread_cond_broadcast(&p->not_full);
        pthread_mutex_unlock(&p->mutex);
        if (n == 0) return total;
        for (int i = 0; i < n; i++) reject(taken[i]);
        total += n;
    }
}

// 제출을 막고 대기 중인 일꾼을 깨운 뒤 큐를 비우고 자원을 해제합니다.
void pool_shutdown(struct pool *p) {
    if (p == NULL) return;

    if (p->mutex_ready) {
        pthread_mutex_lock(&p->mutex);
        p->stopping = 1;
        if (p->not_empty_ready) pthread_cond_broadcast(&p->not_empty);
        if (p->not_full_ready) pthread_cond_broadcast(&p->not_full);
        pthread_mutex_unlock(&p->mutex);
    }

    for (int i = 0; i < p->started_count; i++) {
        pthread_join(p->threads[i], NULL);
    }

    if (p->not_full_ready) pthread_cond_destroy(&p->not_full);
    if (p->not_empty_ready) pthread_cond_destroy(&p->not_empty);
    if (p->mutex_ready) pthread_mutex_destroy(&p->mutex);
    free(p->queue);
    free(p->threads);
    free(p);
}
