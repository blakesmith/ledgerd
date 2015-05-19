#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "ledger.h"

#define MAX_PARTITIONS 65536
#define check_rc(C, R, M) if(!(C)) { \
    rc = R; \
    ctx->last_error = M; \
    goto error; \
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
