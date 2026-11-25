#ifndef CONFIG_H
#define CONFIG_H

struct route;

struct config {
    const char *host;
    int port;
    int backlog;
    const char *root;
    int workers;
    int timeout_sec;
    const char *routes_path;
    int forward_proxy;
    struct route *routes;
    int route_count;
};

// 실행 인자를 읽어 기본 설정을 덮어쓰고, 오류 또는 도움말 여부를 돌려줍니다.
int config_parse(int argc, char **argv, struct config *cfg);
void config_usage(const char *program);

#endif
