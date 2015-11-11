#ifndef LIB_LEDGER_CONSUMER_H
#define LIB_LEDGER_CONSUMER_H

#include <pthread.h>
#include <stdlib.h>

#include "ledger.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum {
    LEDGER_CONSUMER_OK,
    LEDGER_CONSUMER_STOP,
    LEDGER_CONSUMER_ERROR
} ledger_consume_status;

typedef enum {
    LEDGER_FORGET,
    LEDGER_STORE
} ledger_consumer_position_behavior;

typedef struct {
    uint64_t pos;
    pthread_mutex_t lock;
} ledger_consumer_position;

typedef struct {
    size_t read_chunk_size;
    ledger_consumer_position_behavior position_behavior;
    const char *position_key;
} ledger_consumer_options;

typedef struct {
    const char *topic_name;
    unsigned int partition_num;
} ledger_consumer_ctx;

typedef ledger_consume_status (*ledger_consume_function)(ledger_consumer_ctx *ctx, ledger_message_set *messages, void *data);

typedef struct {
    ledger_ctx *ctx;
    ledger_consume_function func;
    ledger_consumer_options options;
    void *data;
    const char *topic_name;
    unsigned int partition_num;
    uint64_t start_id;
    bool active; // TOOD: Needs locking?
    ledger_status status;
    ledger_consumer_position position;
    pthread_t consumer_thread;
    pthread_mutex_t lock;
} ledger_consumer;

typedef struct {
    unsigned int nconsumers;
    unsigned int *partition_ids;
    ledger_consumer *consumers;
} ledger_consumer_group;

// Consumer
ledger_status ledger_init_consumer_options(ledger_consumer_options *options);
ledger_status ledger_consumer_init(ledger_consumer *consumer, ledger_consume_function func,
                                   ledger_consumer_options *options, void *data);
ledger_status ledger_consumer_attach(ledger_consumer *consumer, ledger_ctx *ctx,
                                     const char *topic_name, unsigned int partition_id);
ledger_status ledger_consumer_start(ledger_consumer *consumer, uint64_t start_id);
ledger_status ledger_consumer_stop(ledger_consumer *consumer);
void ledger_consumer_wait(ledger_consumer *consumer);
void ledger_consumer_wait_for_position(ledger_consumer *consumer, uint64_t message_id);
void ledger_consumer_close(ledger_consumer *consumer);


// Consumer position
void ledger_consumer_position_init(ledger_consumer_position *position);
void ledger_consumer_position_set(ledger_consumer_position *position, uint64_t p);
uint64_t ledger_consumer_position_get(ledger_consumer_position *position);

// Consumer group
ledger_status leger_consumer_group_init(ledger_consumer_group *group, unsigned int nconsumers,
                                        ledger_consume_function func, ledger_consumer_options *options,
                                        void *data);

ledger_status ledger_consumer_group_attach(ledger_consumer_group *group, ledger_ctx *ctx,
                                           const char *topic_name, unsigned int *partition_ids);

ledger_status ledger_consumer_group_start(ledger_consumer_group *group);
ledger_status ledger_consumer_group_stop(ledger_consumer_group *group);

void ledger_consumer_group_wait_for_positions(ledger_consumer_group *group, uint64_t *positions, unsigned int npositions);
void ledger_consumer_group_wait(ledger_consumer_group *group);


void ledger_consumer_group_free(ledger_consumer_group *group);

#if defined(__cplusplus)
}
#endif
#endif
