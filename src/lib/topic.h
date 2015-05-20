#ifndef LIB_LEDGER_TOPIC_H
#define LIB_LEDGER_TOPIC_H

#include <stdbool.h>
#include <stdint.h>

#include "partition.h"

#define MAX_PARTITIONS 65536

typedef struct {
    uint32_t length;
    uint32_t crc32;
} ledger_message_hdr;

typedef struct {
    const char *name;
    bool opened;
    ledger_partition *partitions;
    size_t npartitions;
    char *path;
    size_t path_len;
} ledger_topic;

ledger_status ledger_topic_open(ledger_topic *topic, const char *root,
                                const char *name, unsigned int partition_count,
                                int options);
void ledger_topic_close(ledger_topic *topic);
ledger_status ledger_topic_write_partition(ledger_topic *topic, unsigned int partition_num,
                                           void *data, size_t len);

#endif
