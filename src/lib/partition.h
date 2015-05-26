#ifndef LIB_LEDGER_PARTITION_H
#define LIB_LEDGER_PARTITION_H

#include <stdbool.h>
#include <stdint.h>

#include "journal.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
    void *map;
    size_t map_len;
    bool opened;
    uint32_t nentries;
    ledger_journal_meta_entry *entries;
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
ledger_status ledger_partition_read(ledger_partition *partition, uint64_t start_id,
                                    size_t nmessages, bool drop_corrupt, ledger_message_set *messages);

#if defined(__cplusplus)
}
#endif
#endif
