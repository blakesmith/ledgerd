#ifndef LIB_LEDGER_JOURNAL_H
#define LIB_LEDGER_JOURNAL_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

#include "common.h"

typedef struct {
    uint32_t len;
    uint32_t crc32;
} ledger_message_hdr;

typedef struct {
    int fd;
} ledger_journal_index;

typedef struct {
    uint32_t id;
    int fd;
    ledger_journal_index idx;
} ledger_journal;

ledger_status ledger_journal_open(ledger_journal *journal, const char *partition_path,
                                  uint32_t id);
void ledger_journal_close(ledger_journal *journal);
ledger_status ledger_journal_write(ledger_journal *journal, void *data,
                                   size_t len);

#if defined(__cplusplus)
}
#endif
#endif
