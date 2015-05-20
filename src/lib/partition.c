#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include "common.h"
#include "partition.h"

ledger_status ledger_partition_open(ledger_partition *partition, const char *topic_path,
                                    unsigned int partition_number) {
    ledger_status rc;
    char part_num[5];
    ssize_t path_len;
    char *partition_path = NULL;

    partition->path = NULL;

    rc = snprintf(part_num, 5, "%d", partition_number);
    ledger_check_rc(rc > 0, LEDGER_ERR_GENERAL, "Error building partition dir part");

    path_len = ledger_concat_path(topic_path, part_num, &partition_path);
    ledger_check_rc(path_len > 0, path_len, "Failed to construct partition directory path");

    partition->path = partition_path;
    partition->path_len = path_len;

    rc = mkdir(partition_path, 0755);
    ledger_check_rc(rc == 0 || errno == EEXIST, LEDGER_ERR_MKDIR, "Failed to create partition directory");

    return LEDGER_OK;

error:
    if(partition_path) {
        free(partition_path);
    }
    return rc;
}

void ledger_partition_close(ledger_partition *partition) {
    if(partition->path) {
        free(partition->path);
    }
}

