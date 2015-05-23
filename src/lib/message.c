#include <stdlib.h>

#include "common.h"
#include "message.h"

void ledger_message_init(ledger_message *message) {
    message->data = NULL;
    message->len = 0;
}

void ledger_message_free(ledger_message *message) {
    if(message->data) {
        free(message->data);
    }
}

ledger_status ledger_message_set_init(ledger_message_set *messages, size_t nmessages) {
    ledger_status rc;
    int i;
    ledger_message *message;

    messages->messages = NULL;

    messages->nmessages = nmessages;
    messages->messages = ledger_reallocarray(NULL, nmessages, sizeof(ledger_message));
    ledger_check_rc(messages->messages != NULL, LEDGER_ERR_MEMORY, "Failed to allocate message set");

    for(i = 0; i < nmessages; i++) {
        message = messages->messages + i;
        ledger_message_init(message);
    }

    return LEDGER_OK;

error:
    if(messages->messages) {
        free(messages->messages);
    }
    return rc;
}

void ledger_message_set_free(ledger_message_set *messages) {
    ledger_message *message;
    int i;

    if(messages->messages) {
        for(i = 0; i < messages->nmessages; i++) {
            message = messages->messages + i;
            ledger_message_free(message);
        }
        free(messages->messages);
    }
}
