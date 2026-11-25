#include "mime.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void expect(const char *path, const char *type) {
    assert(strcmp(mime_from_path(path), type) == 0);
}

int main(void) {
    expect("a.HTML", "text/html; charset=utf-8");
    expect("a.tar.gz", "application/octet-stream");
    expect("noext", "application/octet-stream");
    expect(".hidden", "application/octet-stream");
    expect("dir.d/file", "application/octet-stream");
    expect("/www/site.css", "text/css; charset=utf-8");
    expect("image.JPEG", "image/jpeg");
    puts("all MIME tests passed");
    return 0;
}
