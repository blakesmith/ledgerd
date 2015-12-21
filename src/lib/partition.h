#ifndef LIB_LEDGER_PARTITION_H
#define LIB_LEDGER_PARTITION_H

#include <stdbool.h>
#include <stdint.h>

#include "signal.h"
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
    pthread_mutex_t rotate_lock;
    pthread_cond_t rotate_cond;
} ledger_partition_locks;

typedef struct {
    void *map;
    size_t map_len;
    bool rotating;
    ledger_partition_locks *locks;
} ledger_partition_lockfile;

typedef struct {
    bool drop_corrupt;
    size_t journal_max_size_bytes;
} ledger_partition_options;

typedef struct {
    unsigned int number;
    bool opened;
    char *path;
    size_t path_len;
    ledger_signal message_signal;
    ledger_partition_options options;
    ledger_partition_meta meta;
    ledger_partition_lockfile lockfile;
} ledger_partition;

ledger_status ledger_partition_open(ledger_partition *partition, const char *topic_path,
                                    unsigned int partition_number, ledger_partition_options *options);
void ledger_partition_close(ledger_partition *partition);
ledger_status ledger_partition_write(ledger_partition *partition, void *data,
                                     size_t len, ledger_write_status *status);
ledger_status ledger_partition_read(ledger_partition *partition, uint64_t start_id,
                                    size_t nmessages, ledger_message_set *messages);
ledger_status ledger_partition_latest_message_id(ledger_partition *partition, uint64_t *id);
void ledger_partition_wait_messages(ledger_partition *partition);
void ledger_partition_signal_readers(ledger_partition *partition);

#if defined(__cplusplus)
}
#endif
#endif
