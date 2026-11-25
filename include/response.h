#ifndef RESPONSE_H
#define RESPONSE_H

#include <stddef.h>

const char *resp_reason(int status);
int resp_format_head(char *buffer, size_t capacity, int status,
                     const char *content_type, long content_length,
                     int keep_alive);
int resp_send_head(int fd, int status, const char *content_type,
                   long content_length, int keep_alive);
int resp_send_simple(int fd, int status, const char *content_type,
                     const char *body, size_t len, int keep_alive);
int resp_send_error(int fd, int status);

#endif
