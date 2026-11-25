#include "route.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// 이 테스트가 만든 규칙 파일을 읽고 라우팅 우선순위와 백엔드 순서를 확인합니다.
int main(void) {
    const char *path = "build/test_routes.conf";
    FILE *file = fopen(path, "w");
    assert(file != NULL);
    fputs("# wildcard fallback\n", file);
    fputs("* /api/ 127.0.0.1:8001 127.0.0.1:8002\n", file);
    fputs("api.example /api/ 127.0.0.1:8003\n", file);
    fputs("api.example /api/v2/ 127.0.0.1:8004\n", file);
    assert(fclose(file) == 0);

    struct route routes[ROUTE_MAX] = {0};
    int count = route_load(path, routes, ROUTE_MAX);
    assert(count == 3);

    struct route *route = route_match(routes, count, "api.example:8080",
                                      "/api/v2/users");
    assert(route == &routes[2]);

    route = route_match(routes, count, "api.example:8080", "/api/users");
    assert(route == &routes[1]);

    route = route_match(routes, count, "other.example:8080", "/api/users");
    assert(route == &routes[0]);

    assert(route_match(routes, count, "other.example", "/static/index.html") == NULL);

    const struct backend *backend = route_pick(&routes[0]);
    assert(backend != NULL && strcmp(backend->port, "8001") == 0);
    backend = route_pick(&routes[0]);
    assert(backend != NULL && strcmp(backend->port, "8002") == 0);
    backend = route_pick(&routes[0]);
    assert(backend != NULL && strcmp(backend->port, "8001") == 0);

    route_destroy(routes, count);
    assert(unlink(path) == 0);
    puts("all route tests passed");
    return 0;
}
