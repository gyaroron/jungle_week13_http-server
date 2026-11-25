#ifndef ROUTE_H
#define ROUTE_H

#include <pthread.h>

#define ROUTE_MAX 64
#define ROUTE_BACKEND_MAX 8

struct backend {
    char host[256];
    char port[8];
};

struct route {
    char host[256];
    char prefix[256];
    struct backend backends[ROUTE_BACKEND_MAX];
    int nbackends;
    unsigned next;
    pthread_mutex_t mutex;
    int mutex_ready;
};

// 라우팅 규칙 파일을 읽고 만든 규칙 수를 돌려줍니다.
int route_load(const char *path, struct route *routes, int max);

// 모든 규칙에서 호스트와 경로 접두사가 맞는 가장 구체적인 규칙을 찾습니다.
struct route *route_match(struct route *routes, int count,
                          const char *host, const char *target);

// 규칙의 백엔드를 mutex로 보호하며 차례대로 선택합니다.
const struct backend *route_pick(struct route *route);

// route_load()가 초기화한 mutex를 없앱니다.
void route_destroy(struct route *routes, int count);

#endif
