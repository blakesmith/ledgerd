#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "common.h"

ssize_t concat_path(const char *s1, const char *s2, char **out) {
    char *buf = NULL;
    size_t l1;
    size_t l2;
    ssize_t path_len;

    l1 = strlen(s1);
    l2 = strlen(s2);
    path_len = l1 + l2 + 2;
    buf = malloc(path_len);
    if(buf == NULL) {
        return LEDGER_ERR_MEMORY;
    }
    memcpy(buf, s1, l1);
    buf[l1] = '/';
    memcpy(buf+l1+1, s2, l2);
    buf[l1+l2+1] = '\0';

    *out = buf;
    return path_len;
}

int ledger_pwrite(int fd, const void *buf, size_t count, off_t offset) {
    ssize_t rv;

    while(count > 0) {
        rv = pwrite(fd, buf, count, offset);
        if(rv == -1 && errno == EINTR) {
            continue;
        }
        if(rv <= 0) {
            return 0;
        }
        count -= rv;
        offset += rv;
    }

    return 1;
}

