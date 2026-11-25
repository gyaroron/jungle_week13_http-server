#include "mime.h"

#include <string.h>
#include <strings.h>

const char *mime_from_path(const char *path) {
    if (path == NULL) return "application/octet-stream";

    const char *base = strrchr(path, '/');
    base = base != NULL ? base + 1 : path;
    const char *dot = strrchr(base, '.');
    if (dot == NULL || dot == base || dot[1] == '\0') {
        return "application/octet-stream";
    }

    const char *ext = dot + 1;
    if (strcasecmp(ext, "html") == 0 || strcasecmp(ext, "htm") == 0)
        return "text/html; charset=utf-8";
    if (strcasecmp(ext, "css") == 0)
        return "text/css; charset=utf-8";
    if (strcasecmp(ext, "js") == 0)
        return "application/javascript; charset=utf-8";
    if (strcasecmp(ext, "json") == 0)
        return "application/json; charset=utf-8";
    if (strcasecmp(ext, "txt") == 0)
        return "text/plain; charset=utf-8";
    if (strcasecmp(ext, "svg") == 0)
        return "image/svg+xml; charset=utf-8";
    if (strcasecmp(ext, "png") == 0)
        return "image/png";
    if (strcasecmp(ext, "jpg") == 0 || strcasecmp(ext, "jpeg") == 0)
        return "image/jpeg";
    if (strcasecmp(ext, "gif") == 0)
        return "image/gif";
    if (strcasecmp(ext, "ico") == 0)
        return "image/x-icon";
    return "application/octet-stream";
}
