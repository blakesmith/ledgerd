#ifndef LIB_LEDGER_MESSAGE_H
#define LIB_LEDGER_MESSAGE_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "common.h"

typedef struct {
    uint32_t len;
    uint32_t crc32;
} ledger_message_hdr;

typedef struct {
    uint64_t id;
    void *data;
    size_t len;
} ledger_message;

typedef struct {
    uint64_t next_id;
    size_t nmessages;
    bool initialized;
    ledger_message *messages;
} ledger_message_set;

ledger_status ledger_message_set_init(ledger_message_set *messages, size_t nmessages);
ledger_status ledger_message_set_grow(ledger_message_set *messages, size_t nmessages);
void ledger_message_set_free(ledger_message_set *messages);

void ledger_message_init(ledger_message *message);
void ledger_message_free(ledger_message *message);

#if defined(__cplusplus)
}
#endif
#endif
