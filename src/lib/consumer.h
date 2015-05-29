#ifndef LIB_LEDGER_CONSUMER_H
#define LIB_LEDGER_CONSUMER_H

#include <stdlib.h>

#include "message.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum {
    LEDGER_CONSUMER_OK
} ledger_consume_status;

typedef struct {
    size_t read_chunk_size;
} ledger_consumer_options;

typedef struct {
    const char *topic_name;
    unsigned int partition_id;
} ledger_consumer_ctx;

typedef ledger_consume_status (*ledger_consume_function)(ledger_consumer_ctx *ctx, ledger_message_set *messages, void *data);

typedef struct {
    ledger_consume_function func;
    ledger_consumer_options options;
    void *data;
    const char *topic_name;
    unsigned int partition_id;
} ledger_consumer;

ledger_status ledger_init_consumer_options(ledger_consumer_options *options);
ledger_status ledger_consumer_init(ledger_consumer *consumer, ledger_consume_function func,
                                   ledger_consumer_options *options, void *data);
ledger_status ledger_consumer_consume_chunk(ledger_consumer *consumer);
void ledger_consumer_close(ledger_consumer *consumer);

#if defined(__cplusplus)
}
#endif
#endif
