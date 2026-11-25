#include "config.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void config_usage(const char *program) {
    fprintf(stderr,
            "Usage: %s [-p port] [-r root] [-b backlog] [-w workers] "
            "[-t seconds] [-c routes-file] [-f] [-h]\n",
            program);
}

// 십진수 인자를 범위 검사 뒤 int 값으로 바꿉니다.
static int parse_number(const char *text, int min, int max, int *out) {
    if (text == NULL || *text == '\0') {
        return -1;
    }
    for (const char *p = text; *p != '\0'; p++) {
        if (*p < '0' || *p > '9') {
            return -1;
        }
    }
    errno = 0;
    char *end = NULL;
    long value = strtol(text, &end, 10);
    if (errno == ERANGE || end == text || *end != '\0' ||
        value < min || value > max || value > INT_MAX) {
        return -1;
    }
    *out = (int)value;
    return 0;
}

int config_parse(int argc, char **argv, struct config *cfg) {
    if (cfg == NULL) {
        return -1;
    }
    *cfg = (struct config){
        .host = "127.0.0.1",
        .port = 8080,
        .backlog = 128,
        .root = "./www",
        .workers = 8,
        .timeout_sec = 5,
        .routes_path = NULL,
        .forward_proxy = 0
    };

    opterr = 0;
    int option;
    while ((option = getopt(argc, argv, "p:r:b:w:t:c:fh")) != -1) {
        switch (option) {
            case 'p':
                if (parse_number(optarg, 1, 65535, &cfg->port) < 0) {
                    fprintf(stderr, "invalid port: %s\n", optarg);
                    config_usage(argv[0]);
                    return -1;
                }
                break;
            case 'r': cfg->root = optarg; break;
            case 'b':
                if (parse_number(optarg, 1, INT_MAX, &cfg->backlog) < 0) {
                    fprintf(stderr, "invalid backlog: %s\n", optarg);
                    config_usage(argv[0]);
                    return -1;
                }
                break;
            case 'w':
                if (parse_number(optarg, 1, 256, &cfg->workers) < 0) {
                    fprintf(stderr, "invalid worker count: %s\n", optarg);
                    config_usage(argv[0]);
                    return -1;
                }
                break;
            case 't':
                if (parse_number(optarg, 1, 3600, &cfg->timeout_sec) < 0) {
                    fprintf(stderr, "invalid timeout: %s\n", optarg);
                    config_usage(argv[0]);
                    return -1;
                }
                break;
            case 'c': cfg->routes_path = optarg; break;
            case 'f': cfg->forward_proxy = 1; break;
            case 'h':
                config_usage(argv[0]);
                return 1;
            default:
                config_usage(argv[0]);
                return -1;
        }
    }
    if (optind != argc) {
        config_usage(argv[0]);
        return -1;
    }
    return 0;
}
