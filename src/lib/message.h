#ifndef LIB_LEDGER_MESSAGE_H
#define LIB_LEDGER_MESSAGE_H

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
    void *data;
    size_t len;
} ledger_message;

typedef struct {
    size_t nmessages;
    ledger_message *messages;
} ledger_message_set;

ledger_status ledger_message_set_init(ledger_message_set *messages, size_t nmessages);
void ledger_message_set_free(ledger_message_set *messages);

void ledger_message_init(ledger_message *message);
void ledger_message_free(ledger_message *message);

#if defined(__cplusplus)
}
#endif
#endif
