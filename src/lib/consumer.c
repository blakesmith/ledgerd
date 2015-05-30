#include "consumer.h"

ledger_status ledger_init_consumer_options(ledger_consumer_options *options) {
    return LEDGER_OK;
}

ledger_status ledger_consumer_init(ledger_consumer *consumer, ledger_consume_function func, ledger_consumer_options *options, void *data) {
    return LEDGER_OK;
}

ledger_status ledger_consumer_consume_chunk(ledger_consumer *consumer) {
    return LEDGER_OK;
}

void ledger_consumer_close(ledger_consumer *consumer) {
}
