/*$OpenBSD: reallocarray.c,v 1.1 2014/05/08 21:43:49 deraadt Exp $*/
/*
 * Copyright (c) 2008 Otto Moerbeek <otto@drijf.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#define _XOPEN_SOURCE 500

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

ssize_t ledger_concat_path(const char *s1, const char *s2, char **out) {
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

/*
 * This is sqrt(SIZE_MAX+1), as s1*s2 <= SIZE_MAX
 * if both s1 < MUL_NO_OVERFLOW and s2 < MUL_NO_OVERFLOW
 */
#define MUL_NO_OVERFLOW (1UL << (sizeof(size_t) * 4))

void *
ledger_reallocarray(void *optr, size_t nmemb, size_t size)
{
    if ((nmemb >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) &&
        nmemb > 0 && SIZE_MAX / nmemb < size) {
        errno = ENOMEM;
        return NULL;
    }
    return realloc(optr, size * nmemb);
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

int ledger_pread(int fd, void *buf, size_t count, off_t offset) {
    ssize_t rv;

    while (count > 0) {
        rv = pread(fd, buf, count, offset);
        if (rv == -1 && errno == EINTR) {
            continue;
        }
        if (rv <= 0) {
            return 0;
        }
        count -= rv;
        offset += rv;
    }

    return 1;
}
