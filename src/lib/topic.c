#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include "common.h"
#include "topic.h"

ledger_status ledger_topic_open(ledger_topic *topic, const char *root,
                                const char *name, unsigned int partition_count,
                                int options) {
    int i;
    char part_num[5];
    ledger_status rc;
    ssize_t path_len;
    char *topic_path = NULL;
    char *partition_path = NULL;

    ledger_check_rc(partition_count < MAX_PARTITIONS, LEDGER_ERR_ARGS, "Too many partitions");

    topic->path = NULL;

    path_len = concat_path(root, name, &topic_path);
    ledger_check_rc(path_len > 0, path_len, "Failed to construct directory path");

    topic->path_len = path_len;
    topic->path = topic_path;
    topic->npartitions = partition_count;

    rc = mkdir(topic_path, 0755);
    ledger_check_rc(rc == 0 || errno == EEXIST, LEDGER_ERR_MKDIR, "Failed to create context directory");

    for(i = 0; i < partition_count; i++) {
        rc = snprintf(part_num, 5, "%d", i);
        ledger_check_rc(rc > 0, LEDGER_ERR_GENERAL, "Error building partition dir part");

        path_len = concat_path(topic_path, part_num, &partition_path);
        ledger_check_rc(path_len > 0, path_len, "Failed to construct partition directory path");

        rc = mkdir(partition_path, 0755);
        ledger_check_rc(rc == 0 || errno == EEXIST, LEDGER_ERR_MKDIR, "Failed to create partition directory");

        free(partition_path);
    }

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

void ledger_topic_close(ledger_topic *topic) {
    if(topic->path) {
        free(topic->path);
    }
}
