#define _DARWIN_C_SOURCE
#define _XOPEN_SOURCE 700

#include "cgi.h"

#include "net.h"
#include "response.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <limits.h>

#define CGI_OUTPUT_LIMIT 1048576

static int cgi_error(int fd, int status) {
    return resp_send_error(fd, status) < 0 ? -1 : 1;
}

static int cgi_hex(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

static int cgi_decode_path(const char *target, char *path, size_t cap,
                           const char **query_out) {
    const char *query = strchr(target, '?');
    size_t len = query != NULL ? (size_t)(query - target) : strlen(target);
    if (query_out != NULL) *query_out = query != NULL ? query + 1 : "";
    if (len == 0 || target[0] != '/') return 400;
    size_t out = 0;
    for (size_t i = 0; i < len; i++) {
        unsigned char ch = (unsigned char)target[i];
        if (target[i] == '%') {
            if (i + 2 >= len) return 400;
            int hi = cgi_hex(target[i + 1]);
            int lo = cgi_hex(target[i + 2]);
            if (hi < 0 || lo < 0) return 400;
            ch = (unsigned char)((hi << 4) | lo);
            i += 2;
            if (ch == 0) return 400;
        }
        if (out + 1 >= cap) return 414;
        path[out++] = (char)ch;
    }
    path[out] = '\0';
    for (const char *part = path; *part != '\0';) {
        while (*part == '/') part++;
        const char *end = strchr(part, '/');
        size_t part_len = end != NULL ? (size_t)(end - part) : strlen(part);
        if (part_len == 2 && part[0] == '.' && part[1] == '.') return 403;
        if (end == NULL) break;
        part = end + 1;
    }
    return 0;
}

static int path_inside(const char *root, const char *file) {
    size_t n = strlen(root);
    return strncmp(root, file, n) == 0 &&
           (file[n] == '\0' || file[n] == '/');
}

static int write_all(int fd, const char *buf, size_t len) {
    size_t done = 0;
    while (done < len) {
        ssize_t n = write(fd, buf + done, len - done);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return -1;
        done += (size_t)n;
    }
    return 0;
}

static int find_header_end(const char *buf, size_t len, size_t *end_out) {
    for (size_t i = 0; i + 3 < len; i++) {
        if (memcmp(buf + i, "\r\n\r\n", 4) == 0) {
            *end_out = i + 4;
            return 1;
        }
    }
    return 0;
}

int cgi_serve(int fd, const struct config *cfg, const struct http_request *req,
              const char *body, size_t body_len) {
    if (strcmp(req->method, "GET") != 0 && strcmp(req->method, "POST") != 0)
        return cgi_error(fd, 405);

    char decoded[PATH_MAX];
    const char *query = "";
    int path_result = cgi_decode_path(req->target, decoded, sizeof(decoded), &query);
    if (path_result != 0) return cgi_error(fd, path_result);

    char root[PATH_MAX];
    if (realpath(cfg->root, root) == NULL) return cgi_error(fd, 500);
    char candidate[PATH_MAX];
    int n = snprintf(candidate, sizeof(candidate), "%s/%s", root, decoded + 1);
    if (n < 0 || (size_t)n >= sizeof(candidate)) return cgi_error(fd, 414);
    char script[PATH_MAX];
    if (realpath(candidate, script) == NULL) {
        if (errno == ENOENT) return cgi_error(fd, 404);
        if (errno == EACCES) return cgi_error(fd, 403);
        return cgi_error(fd, 500);
    }
    if (!path_inside(root, script)) return cgi_error(fd, 403);
    struct stat st;
    if (stat(script, &st) < 0) {
        if (errno == EACCES) return cgi_error(fd, 403);
        if (errno == ENOENT) return cgi_error(fd, 404);
        return cgi_error(fd, 500);
    }
    if (!S_ISREG(st.st_mode) || access(script, X_OK) < 0)
        return cgi_error(fd, 403);

    int input_pipe[2];
    int output_pipe[2];

    // fork() 전에 CGI 환경 문자열을 준비합니다.
    // 여러 스레드에서 호출해도 자식이 fork() 뒤에 snprintf()를 실행하지 않게 합니다.
    char content_length[32];
    int content_length_result = snprintf(content_length, sizeof(content_length), "%zu", body_len);
    const char *request_content_type = http_header_get(req, "Content-Type");
    char env_method[64], env_query[4096], env_length[64], env_type[1200];
    char env_script[PATH_MAX + 32], env_protocol[32];
    int env_results[] = {
        snprintf(env_method, sizeof(env_method), "REQUEST_METHOD=%s", req->method),
        snprintf(env_query, sizeof(env_query), "QUERY_STRING=%s", query),
        snprintf(env_length, sizeof(env_length), "CONTENT_LENGTH=%s", content_length),
        snprintf(env_type, sizeof(env_type), "CONTENT_TYPE=%s",
                 request_content_type != NULL ? request_content_type : ""),
        snprintf(env_script, sizeof(env_script), "SCRIPT_NAME=%s", decoded),
        snprintf(env_protocol, sizeof(env_protocol), "SERVER_PROTOCOL=%s", req->version)
    };
    size_t env_capacities[] = {
        sizeof(env_method), sizeof(env_query), sizeof(env_length), sizeof(env_type),
        sizeof(env_script), sizeof(env_protocol)
    };
    if (content_length_result < 0 || (size_t)content_length_result >= sizeof(content_length)) {
        return cgi_error(fd, 500);
    }
    for (size_t i = 0; i < sizeof(env_results) / sizeof(env_results[0]); i++) {
        if (env_results[i] < 0 || (size_t)env_results[i] >= env_capacities[i]) {
            return cgi_error(fd, 500);
        }
    }
    char *envp[] = {env_method, env_query, env_length, env_type, env_script,
                    env_protocol, "GATEWAY_INTERFACE=CGI/1.1",
                    "PATH=/usr/bin:/bin", NULL};

    if (pipe(input_pipe) < 0) return cgi_error(fd, 500);
    if (pipe(output_pipe) < 0) {
        close(input_pipe[0]); close(input_pipe[1]);
        return cgi_error(fd, 500);
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(input_pipe[0]); close(input_pipe[1]);
        close(output_pipe[0]); close(output_pipe[1]);
        return cgi_error(fd, 500);
    }
    if (pid == 0) {
        if (dup2(input_pipe[0], STDIN_FILENO) < 0 ||
            dup2(output_pipe[1], STDOUT_FILENO) < 0) _exit(127);
        close(input_pipe[0]); close(input_pipe[1]);
        close(output_pipe[0]); close(output_pipe[1]);

        char *argv[] = {script, NULL};
        execve(script, argv, envp);
        _exit(127);
    }

    close(input_pipe[0]);
    close(output_pipe[1]);
    if (body_len > 0 && write_all(input_pipe[1], body, body_len) < 0) {
        close(input_pipe[1]); close(output_pipe[0]);
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        return cgi_error(fd, 500);
    }
    close(input_pipe[1]);

    char *output = malloc(CGI_OUTPUT_LIMIT + 1);
    if (output == NULL) {
        close(output_pipe[0]);
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        return cgi_error(fd, 500);
    }
    size_t output_len = 0;
    int timed_out = 0;
    int too_large = 0;
    for (;;) {
        int ready = net_wait(output_pipe[0], POLLIN, cfg->timeout_sec * 1000);
        if (ready == 0) { timed_out = 1; break; }
        if (ready < 0 && errno == EINTR) continue;
        if (ready < 0) break;
        ssize_t got = read(output_pipe[0], output + output_len,
                           CGI_OUTPUT_LIMIT - output_len);
        if (got < 0 && errno == EINTR) continue;
        if (got < 0) break;
        if (got == 0) break;
        output_len += (size_t)got;
        if (output_len == CGI_OUTPUT_LIMIT) {
            char extra;
            ssize_t more = read(output_pipe[0], &extra, 1);
            if (more > 0) too_large = 1;
            break;
        }
    }
    close(output_pipe[0]);
    if (timed_out || too_large) kill(pid, SIGKILL);
    int child_status = 0;
    while (waitpid(pid, &child_status, 0) < 0) {
        if (errno != EINTR) { free(output); return cgi_error(fd, 500); }
    }
    if (timed_out) { free(output); return cgi_error(fd, 504); }
    if (too_large || !WIFEXITED(child_status) || WEXITSTATUS(child_status) != 0) {
        free(output);
        return cgi_error(fd, 500);
    }

    size_t header_end = 0;
    if (!find_header_end(output, output_len, &header_end)) {
        free(output);
        return cgi_error(fd, 500);
    }
    char content_type[256] = "text/plain; charset=utf-8";
    size_t line = 0;
    while (line + 1 < header_end) {
        size_t end = line;
        while (end + 1 < header_end &&
               !(output[end] == '\r' && output[end + 1] == '\n')) end++;
        if (end == line) break;
        const char *colon = memchr(output + line, ':', end - line);
        if (colon != NULL) {
            size_t name_len = (size_t)(colon - (output + line));
            if (name_len == 12 && strncasecmp(output + line, "Content-Type", 12) == 0) {
                size_t value = (size_t)(colon - output) + 1;
                while (value < end && (output[value] == ' ' || output[value] == '\t')) value++;
                size_t value_len = end - value;
                if (value_len >= sizeof(content_type)) value_len = sizeof(content_type) - 1;
                memcpy(content_type, output + value, value_len);
                content_type[value_len] = '\0';
            }
        }
        line = end + 2;
    }
    size_t body_start = header_end;
    int send_result = resp_send_simple(fd, 200, content_type,
                                       output + body_start, output_len - body_start,
                                       req->keep_alive);
    free(output);
    return send_result < 0 ? -1 : 0;
}
