#include <string.h>

#include "consumer.h"

static void *consumer_loop(void *consumer_ptr) {
    ledger_status rc;
    ledger_message_set messages;
    ledger_consumer_ctx ctx;
    ledger_consumer *consumer = (ledger_consumer *)consumer_ptr;
    uint64_t next_message = consumer->start_id;
    uint64_t last_pos;

    ctx.topic_name = consumer->topic_name;
    ctx.partition_num = consumer->partition_num;

    rc = ledger_read_partition(consumer->ctx, consumer->topic_name,
                               consumer->partition_num, next_message,
                               consumer->options.read_chunk_size, &messages);

    if(rc != LEDGER_OK) {
        // TODO: Somehow propagate up to the user?
    }

    consumer->func(&ctx, &messages, consumer->data);

    next_message = messages.next_id;
    last_pos = messages.messages[messages.nmessages-1].id;
    ledger_consumer_position_set(&consumer->position, last_pos);

    ledger_message_set_free(&messages);
    return NULL;
}

ledger_status ledger_init_consumer_options(ledger_consumer_options *options) {
    return LEDGER_OK;
}

ledger_status ledger_consumer_init(ledger_consumer *consumer, ledger_consume_function func, ledger_consumer_options *options, void *data) {
    consumer->func = func;
    consumer->data = data;
    ledger_consumer_position_init(&consumer->position);
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
    ledger_status rc;

    consumer->start_id = start_id;

    rc = pthread_create(&consumer->consumer_thread, NULL, consumer_loop, consumer);
    ledger_check_rc(rc == 0, LEDGER_ERR_GENERAL, "Failed to launch consumer thread");

    return LEDGER_OK;

error:
    return rc;
}

ledger_status ledger_consumer_stop(ledger_consumer *consumer) {
    return LEDGER_OK;
}

void ledger_consumer_close(ledger_consumer *consumer) {
}

void ledger_consumer_position_init(ledger_consumer_position *position) {
    pthread_mutex_init(&position->lock, NULL);
}

void ledger_consumer_position_set(ledger_consumer_position *position, uint64_t p) {
    pthread_mutex_lock(&position->lock);
    position->pos = p;
    pthread_mutex_unlock(&position->lock);
}

uint64_t ledger_consumer_position_get(ledger_consumer_position *position) {
    uint64_t p;

    pthread_mutex_lock(&position->lock);
    p = position->pos;
    pthread_mutex_unlock(&position->lock);
    return p;
}
