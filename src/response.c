#include "response.h"

#include "net.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

const char *resp_reason(int status) {
    switch (status) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 408: return "Request Timeout";
        case 413: return "Payload Too Large";
        case 414: return "URI Too Long";
        case 431: return "Request Header Fields Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 504: return "Gateway Timeout";
        case 505: return "HTTP Version Not Supported";
        default: return "Internal Server Error";
    }
}

int resp_send_head(int fd, int status, const char *content_type,
                   long content_length, int keep_alive) {
    char date[64] = "Thu, 01 Jan 1970 00:00:00 GMT";
    time_t now = time(NULL);
    struct tm utc;
    if (now != (time_t)-1 && gmtime_r(&now, &utc) != NULL) {
        strftime(date, sizeof(date), "%a, %d %b %Y %H:%M:%S GMT", &utc);
    }

    char header[1024];
    int length = snprintf(header, sizeof(header),
                          "HTTP/1.1 %d %s\r\n"
                          "Content-Type: %s\r\n"
                          "Content-Length: %ld\r\n"
                          "Connection: %s\r\n"
                          "Date: %s\r\n"
                          "Server: c-webserver-proxy/1.0\r\n\r\n",
                          status, resp_reason(status),
                          content_type != NULL ? content_type : "application/octet-stream",
                          content_length, keep_alive ? "keep-alive" : "close", date);
    if (length < 0 || (size_t)length >= sizeof(header)) {
        static const char fallback[] =
            "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
        return net_send_all(fd, fallback, sizeof(fallback) - 1);
    }
    return net_send_all(fd, header, (size_t)length);
}

int resp_send_simple(int fd, int status, const char *content_type,
                     const char *body, size_t len, int keep_alive) {
    if (len > 0 && body == NULL) return -1;
    if (resp_send_head(fd, status, content_type, (long)len, keep_alive) < 0) {
        return -1;
    }
    return len > 0 ? net_send_all(fd, body, len) : 0;
}

int resp_send_error(int fd, int status) {
    char body[256];
    int length = snprintf(body, sizeof(body),
                          "<!doctype html><html><body><h1>%d %s</h1></body></html>\n",
                          status, resp_reason(status));
    if (length < 0 || (size_t)length >= sizeof(body)) return -1;
    return resp_send_simple(fd, status, "text/html; charset=utf-8",
                            body, (size_t)length, 0);
}
