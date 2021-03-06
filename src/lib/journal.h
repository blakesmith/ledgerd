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

#define LEDGER_BEGIN 0
#define LEDGER_END UINT64_MAX
#define LEDGER_CHUNK_SIZE 64

typedef struct {
    uint32_t id;
    uint64_t first_message_id;
    uint64_t create_time;
    pthread_mutex_t write_lock;
} ledger_journal_meta_entry;

typedef struct {
    int fd;
    void *map;
    size_t map_len;
} ledger_journal_index;

typedef struct {
    bool drop_corrupt;
    size_t max_size_bytes;
} ledger_journal_options;

typedef struct {
    int fd;
    ledger_journal_options options;
    ledger_journal_index idx;
    ledger_journal_meta_entry *metadata;
} ledger_journal;

typedef struct {
    uint64_t message_id;
    unsigned int partition_num;
} ledger_write_status;

ledger_status ledger_journal_open(ledger_journal *journal, const char *partition_path,
                                  ledger_journal_meta_entry *metadata, ledger_journal_options *options);
void ledger_journal_close(ledger_journal *journal);
ledger_status ledger_journal_write(ledger_journal *journal, void *data,
                                   size_t len, ledger_write_status *status);
ledger_status ledger_journal_latest_message_id(ledger_journal *journal, uint64_t *id);
ledger_status ledger_journal_read(ledger_journal *journal, uint64_t start_id,
                                  size_t nmessages, ledger_message_set *messages);
ledger_status ledger_journal_delete(const char *partition_path, uint32_t journal_id);

#if defined(__cplusplus)
}
#endif
#endif
