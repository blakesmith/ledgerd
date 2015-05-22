#ifndef LIB_LEDGER_PARTITION_H
#define LIB_LEDGER_PARTITION_H

#include <stdbool.h>
#include <stdint.h>

#include "message.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
    uint32_t id;
    uint64_t first_journal_id;
    uint64_t first_journal_time;
} ledger_partition_meta_entry;

typedef struct {
    void *map;
    size_t map_len;
    bool opened;
    uint32_t nentries;
    ledger_partition_meta_entry *entries;
} ledger_partition_meta;

typedef struct {
    unsigned int number;
    bool opened;
    char *path;
    size_t path_len;
    ledger_partition_meta meta;
} ledger_partition;

ledger_status ledger_partition_open(ledger_partition *partition, const char *topic_path,
                                    unsigned int partition_number);
void ledger_partition_close(ledger_partition *partition);
ledger_status ledger_partition_write(ledger_partition *partition, void *data,
                                     size_t len);

#if defined(__cplusplus)
}
#endif
#endif
