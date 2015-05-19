#ifndef LIB_LEDGER_TOPIC_H
#define LIB_LEDGER_TOPIC_H

#include <stdint.h>

#define MAX_PARTITIONS 65536

typedef struct {
    uint32_t length;
    uint32_t crc32;
} ledger_message_hdr;

typedef struct {
    uint64_t first_message_id;
    uint64_t first_message_time;
} ledger_partition_index_hdr;

typedef struct {
    ledger_partition_index_hdr hdr;
    int fd;
} ledger_partition_index;

typedef struct {
    ledger_partition_index idx;
} ledger_partition;
  
typedef struct {
    const char *name;
    ledger_partition *partitions;
    size_t npartitions;
    char *path;
    size_t path_len;
} ledger_topic;

ledger_status ledger_topic_open(ledger_topic *topic, const char *root,
                                const char *name, unsigned int partition_count,
                                int options);
void ledger_topic_close(ledger_topic *topic);

#endif
