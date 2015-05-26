#ifndef LIB_LEDGER_JOURNAL_H
#define LIB_LEDGER_JOURNAL_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include "common.h"
#include "message.h"

typedef struct {
    uint32_t id;
    uint64_t first_journal_id;
    uint64_t first_journal_time;
    pthread_mutex_t partition_write_lock;
} ledger_journal_meta_entry;

typedef struct {
    int fd;
    void *map;
    size_t map_len;
} ledger_journal_index;

typedef struct {
    int fd;
    ledger_journal_index idx;
    ledger_journal_meta_entry *metadata;
} ledger_journal;

ledger_status ledger_journal_open(ledger_journal *journal, const char *partition_path,
                                  ledger_journal_meta_entry *metadata);
void ledger_journal_close(ledger_journal *journal);
ledger_status ledger_journal_write(ledger_journal *journal, void *data,
                                   size_t len);
ledger_status ledger_journal_read(ledger_journal *journal, uint64_t start_id,
                                  size_t nmessages, bool drop_corrupt, ledger_message_set *messages);

#if defined(__cplusplus)
}
#endif
#endif
