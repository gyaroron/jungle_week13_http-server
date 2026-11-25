#ifndef CGI_H
#define CGI_H

#include "config.h"
#include "http.h"

int cgi_serve(int fd, const struct config *cfg, const struct http_request *req,
              const char *body, size_t body_len);

#endif
