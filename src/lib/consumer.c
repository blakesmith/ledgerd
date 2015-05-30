#include <string.h>

#include "consumer.h"

static void *consumer_loop(void *consumer_ptr) {
    ledger_status rc;
    ledger_message_set messages;
    ledger_consumer_ctx ctx;
    ledger_consumer *consumer = (ledger_consumer *)consumer_ptr;

    ctx.topic_name = consumer->topic_name;
    ctx.partition_num = consumer->partition_num;

    rc = ledger_read_partition(consumer->ctx, consumer->topic_name,
                               consumer->partition_num, consumer->start_id,
                               consumer->options.read_chunk_size, &messages);

    if(rc != LEDGER_OK) {
        // TODO: Somehow propagate up to the user?
    }

    consumer->func(&ctx, &messages, consumer->data);

    return NULL;
}

ledger_status ledger_init_consumer_options(ledger_consumer_options *options) {
    return LEDGER_OK;
}

ledger_status ledger_consumer_init(ledger_consumer *consumer, ledger_consume_function func, ledger_consumer_options *options, void *data) {
    consumer->func = func;
    consumer->data = data;
    memcpy(&consumer->options, options, sizeof(ledger_consumer_options));

    return LEDGER_OK;
}

ledger_status ledger_consumer_attach(ledger_consumer *consumer, ledger_ctx *ctx,
                                     const char *topic_name, unsigned int partition_num) {
    consumer->ctx = ctx;
    consumer->topic_name = topic_name;
    consumer->partition_num = partition_num;

    return LEDGER_OK;
}

ledger_status ledger_consumer_start(ledger_consumer *consumer, uint64_t start_id) {
    consumer->start_id = start_id;

    consumer_loop(consumer);

    return LEDGER_OK;
}

ledger_status ledger_consumer_stop(ledger_consumer *consumer) {
    return LEDGER_OK;
}

void ledger_consumer_close(ledger_consumer *consumer) {
}
