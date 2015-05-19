#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ledger.h"

#define MAX_PARTITIONS 65536
#define check_rc(C, R, M) if(!(C)) { \
    rc = R; \
    ctx->last_error = M; \
    goto error; \
}

static int ledger_pwrite(int fd, const void *buf, size_t count, off_t offset) {
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

static ssize_t concat_path(const char *s1, const char *s2, char **out) {
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

static ledger_status open_latest_partition_index(ledger_ctx *ctx, const char *topic, unsigned int partition_num, ledger_partition_index *idx) {
    return LEDGER_OK;
}

static ledger_status open_latest_partition(ledger_ctx *ctx, const char *topic, unsigned int partition_num, ledger_partition *partition) {
    ledger_status rc;

    rc = open_latest_partition_index(ctx, topic, partition_num, &partition->idx);
    check_rc(rc == LEDGER_OK, LEDGER_ERR_GENERAL, "Failed to open latest partition index");

    return LEDGER_OK;
error:
    return rc;
}

const char *ledger_err(ledger_ctx *ctx) {
    return ctx->last_error;
}

ledger_status ledger_open_context(ledger_ctx *ctx, const char *root_directory) {
    ctx->root_directory = root_directory;
    return LEDGER_OK;
}

void ledger_close_context(ledger_ctx *ctx) {
}

ledger_status ledger_open_topic(ledger_ctx *ctx, const char *topic,
                                unsigned int partition_count, int options) {
    int i;
    char part_num[5];
    ledger_status rc;
    ssize_t path_len;
    char *topic_path = NULL;
    char *partition_path = NULL;

    check_rc(partition_count < MAX_PARTITIONS, LEDGER_ERR_ARGS, "Too many partitions");

    path_len = concat_path(ctx->root_directory, topic, &topic_path);
    check_rc(path_len > 0, path_len, "Failed to construct directory path");

    rc = mkdir(topic_path, 0755);
    check_rc(rc == 0 || errno == EEXIST, LEDGER_ERR_MKDIR, "Failed to create context directory");

    for(i = 0; i < partition_count; i++) {
        rc = snprintf(part_num, 5, "%d", i);
        check_rc(rc > 0, LEDGER_ERR_GENERAL, "Error building partition dir part");

        path_len = concat_path(topic_path, part_num, &partition_path);
        check_rc(path_len > 0, path_len, "Failed to construct partition directory path");

        rc = mkdir(partition_path, 0755);
        check_rc(rc == 0 || errno == EEXIST, LEDGER_ERR_MKDIR, "Failed to create partition directory");

        free(partition_path);
    }

    free(topic_path);
    return LEDGER_OK;

error:
    if(topic_path) {
        free(topic_path);
    }
    if(partition_path) {
        free(partition_path);
    }
    return rc;
}

ledger_status ledger_write_partition(ledger_ctx *ctx, const char *topic,
                                     unsigned int partition_num, void *data,
                                     size_t len) {
    ledger_status rc;
    ledger_partition partition;

    rc = open_latest_partition(ctx, topic, partition_num, &partition);
    check_rc(rc == LEDGER_OK, LEDGER_ERR_GENERAL, "Failed to open latest partition");

    return LEDGER_OK;

error:
    return rc;
}
