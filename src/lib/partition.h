#ifndef LIB_LEDGER_PARTITION_H
#define LIB_LEDGER_PARTITION_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    void *data;
    size_t len;
} ledger_message;

typedef struct {
    size_t nmessages;
    ledger_message *messages;
} ledger_message_set;

typedef struct {
    uint64_t first_message_id;
    uint64_t first_message_time;
} ledger_partition_index_hdr;

typedef struct {
    ledger_partition_index_hdr hdr;
    int fd;
} ledger_partition_index;

typedef struct {
    unsigned int number;
    bool opened;
    char *path;
    size_t path_len;
    ledger_partition_index idx;
} ledger_partition;

ledger_status ledger_partition_open(ledger_partition *partition, const char *topic_path,
                                    unsigned int partition_number);
void ledger_partition_close(ledger_partition *partition);

#endif
