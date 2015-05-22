#ifndef LIB_LEDGER_TOPIC_H
#define LIB_LEDGER_TOPIC_H

#include <stdbool.h>
#include <stdint.h>

#include "partition.h"

#define MAX_PARTITIONS 65536

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
ledger_status ledger_topic_read_partition(ledger_topic *topic, unsigned int partition_num,
                                          uint64_t last_id, size_t nmessages,
                                          ledger_message_set *messages);

#endif
