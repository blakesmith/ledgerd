#ifndef LIB_LEDGER_TOPIC_H
#define LIB_LEDGER_TOPIC_H

#include <stdbool.h>
#include <stdint.h>

#include "partition.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define MAX_PARTITIONS 65536
#define DEFAULT_JOURNAL_MAX_SIZE 536870912 // 536.871 megabytes

typedef struct {
    bool drop_corrupt;
    size_t journal_max_size_bytes;
} ledger_topic_options;

typedef struct {
    const char *name;
    bool opened;
    ledger_topic_options options;
    ledger_partition *partitions;
    size_t npartitions;
    char *path;
    size_t path_len;
} ledger_topic;

ledger_status ledger_topic_options_init(ledger_topic_options *options);
ledger_status ledger_topic_open(ledger_topic *topic, const char *root,
                                const char *name, unsigned int partition_count,
                                ledger_topic_options *options);
void ledger_topic_close(ledger_topic *topic);
ledger_status ledger_topic_write_partition(ledger_topic *topic, unsigned int partition_num,
                                           void *data, size_t len, ledger_write_status *status);
ledger_status ledger_topic_read_partition(ledger_topic *topic, unsigned int partition_num,
                                          uint64_t start_id, size_t nmessages,
                                          ledger_message_set *messages);

ledger_status ledger_topic_wait_messages(ledger_topic *topic, unsigned int partition_num);
ledger_status ledger_topic_signal_readers(ledger_topic *topic, unsigned int partition_num);

#if defined(__cplusplus)
}
#endif
#endif
