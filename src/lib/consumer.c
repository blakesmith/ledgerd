#define _POSIX_C_SOURCE 199309L

#include <assert.h>
#include <string.h>
#include <time.h>

#include "consumer.h"

#define DEFAULT_READ_CHUNK_SIZE 64

static void *consumer_loop(void *consumer_ptr) {
    ledger_status rc;
    ledger_message_set messages;
    ledger_consumer_ctx ctx;
    ledger_consumer *consumer = (ledger_consumer *)consumer_ptr;
    ledger_consume_status consume_status;
    uint64_t next_message = consumer->start_id;
    uint64_t last_pos;

    ctx.topic_name = consumer->topic_name;
    ctx.partition_num = consumer->partition_num;

    while(consumer->active) {
        rc = ledger_read_partition(consumer->ctx, consumer->topic_name,
                                   consumer->partition_num, next_message,
                                   consumer->options.read_chunk_size, &messages);

        if(rc != LEDGER_OK) {
            consumer->status = rc;
            ledger_consumer_stop(consumer);
            return NULL;
        }

        if(messages.nmessages == 0) {
            if(next_message == LEDGER_END) {
                last_pos = next_message;
                next_message = messages.next_id;
                ledger_consumer_position_set(&consumer->position, last_pos);

                if(consumer->options.position_behavior == LEDGER_STORE) {
                    rc = ledger_position_storage_set(&consumer->ctx->position_storage,
                                                     consumer->options.position_key,
                                                     consumer->partition_num,
                                                     next_message);
                    if(rc != LEDGER_OK) {
                        consumer->status = rc;
                        ledger_consumer_stop(consumer);

                        return NULL;
                    }
                }
            }
            ledger_message_set_free(&messages);
            ledger_wait_messages(consumer->ctx, consumer->topic_name,
                                 consumer->partition_num);
            continue;
        }

        if(rc == LEDGER_OK) {
            consume_status = consumer->func(&ctx, &messages, consumer->data);
            if(consume_status == LEDGER_CONSUMER_ERROR) {
                ledger_message_set_free(&messages);
                return NULL;
            }

            next_message = messages.next_id;
            last_pos = messages.messages[messages.nmessages-1].id;
            ledger_consumer_position_set(&consumer->position, last_pos);

            if(consumer->options.position_behavior == LEDGER_STORE) {
                rc = ledger_position_storage_set(&consumer->ctx->position_storage,
                                                 consumer->options.position_key,
                                                 consumer->partition_num,
                                                 next_message);
                if(rc != LEDGER_OK) {
                    consumer->status = rc;
                    ledger_consumer_stop(consumer);
                    return NULL;
                }
            }

            ledger_message_set_free(&messages);

            if(consume_status == LEDGER_CONSUMER_STOP) {
                ledger_consumer_stop(consumer);
            }
        }
    }
    return NULL;
}

ledger_status ledger_init_consumer_options(ledger_consumer_options *options) {
    options->read_chunk_size = DEFAULT_READ_CHUNK_SIZE;
    options->position_behavior = LEDGER_FORGET;
    options->position_key = NULL;
    return LEDGER_OK;
}

ledger_status ledger_consumer_init(ledger_consumer *consumer, ledger_consume_function func, ledger_consumer_options *options, void *data) {
    consumer->func = func;
    consumer->data = data;
    consumer->active = false;
    consumer->status = LEDGER_OK;
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

    if(consumer->options.position_behavior == LEDGER_STORE) {
        ledger_check_rc(consumer->options.position_key != NULL, LEDGER_ERR_ARGS, "You must set a position key for stable storage");
        rc = ledger_position_storage_get(&consumer->ctx->position_storage,
                                         consumer->options.position_key,
                                         consumer->partition_num,
                                         &consumer->start_id);
        ledger_check_rc(rc == LEDGER_OK || rc == LEDGER_ERR_POSITION_NOT_FOUND, rc, "Failed to fetch consumer position");

        if(rc == LEDGER_ERR_POSITION_NOT_FOUND) {
            consumer->start_id = start_id;
        }
    } else {
        consumer->start_id = start_id;
    }

    rc = pthread_create(&consumer->consumer_thread, NULL, consumer_loop, consumer);
    ledger_check_rc(rc == 0, LEDGER_ERR_GENERAL, "Failed to launch consumer thread");

    consumer->active = true;

    return LEDGER_OK;

error:
    return rc;
}

ledger_status ledger_consumer_stop(ledger_consumer *consumer) {
    consumer->active = false;
    ledger_signal_readers(consumer->ctx, consumer->topic_name,
                          consumer->partition_num);
    return LEDGER_OK;
}

void ledger_consumer_wait(ledger_consumer *consumer) {
    // 2 milliseconds
    static const struct timespec time = {
        0,
        2 * 1000L * 1000L,
    };

    while(consumer->active) {
        ledger_signal_readers(consumer->ctx, consumer->topic_name,
                              consumer->partition_num);
        nanosleep(&time, NULL);
    }

   pthread_join(consumer->consumer_thread, NULL);
}

void ledger_consumer_wait_for_position(ledger_consumer *consumer, uint64_t message_id) {
    // 2 milliseconds
    static const struct timespec time = {
        0,
        2 * 1000L * 1000L,
    };
    uint64_t pos;

    do {
        pos = ledger_consumer_position_get(&consumer->position);
        nanosleep(&time, NULL);
    } while(pos < message_id || pos == LEDGER_END);
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

ledger_status ledger_consumer_group_init(ledger_consumer_group *group, unsigned int nconsumers,
                                         ledger_consume_function func, ledger_consumer_options *options,
                                         void *data) {
    ledger_status rc;
    int i;

    group->nconsumers = nconsumers;
    group->consumers = ledger_reallocarray(group->consumers,
                                           nconsumers,
                                           sizeof(ledger_consumer));
    ledger_check_rc(group->consumers != NULL, LEDGER_ERR_MEMORY, "Failed to allocate consumers");

    for(i = 0; i < nconsumers; i++) {
        rc = ledger_consumer_init(&group->consumers[i], func, options, data);
        ledger_check_rc(rc == LEDGER_OK, rc, "Failed to init consumer");
    }

    return LEDGER_OK;

error:
    return rc;
}

ledger_status ledger_consumer_group_attach(ledger_consumer_group *group, ledger_ctx *ctx,
                                           const char *topic_name, unsigned int *partition_ids) {
    ledger_status rc = LEDGER_OK;
    int i;

    for(i = 0; i < group->nconsumers; i++) {
        rc = ledger_consumer_attach(&group->consumers[i], ctx, topic_name, partition_ids[i]);
        ledger_check_rc(rc == LEDGER_OK, rc, "Failed to attach consumer from group");
    }

    return LEDGER_OK;

error:
    return rc;
}

ledger_status ledger_consumer_group_start(ledger_consumer_group *group) {
    ledger_status rc = LEDGER_OK;
    int i;

    for(i = 0; i < group->nconsumers; i++) {
        rc = ledger_consumer_start(&group->consumers[i], LEDGER_BEGIN);
        ledger_check_rc(rc == LEDGER_OK, rc, "Failed to start consumer in group");
    }

    return LEDGER_OK;

error:
    return rc;
}

ledger_status ledger_consumer_group_stop(ledger_consumer_group *group) {
    ledger_status rc = LEDGER_OK;
    int i;

    for(i = 0; i < group->nconsumers; i++) {
        rc = ledger_consumer_stop(&group->consumers[i]);
        ledger_check_rc(rc == LEDGER_OK, rc, "Failed to start consumer in group");
    }

    return LEDGER_OK;

error:
    return rc;
}

void ledger_consumer_group_wait_for_positions(ledger_consumer_group *group, uint64_t *positions, unsigned int npositions) {
    int i;

    assert(npositions == group->nconsumers);

    for(i = 0; i < group->nconsumers; i++) {
        ledger_consumer_wait_for_position(&group->consumers[i], positions[i]);
    }
}

void ledger_consumer_group_wait(ledger_consumer_group *group) {
    int i;

    for(i = 0; i < group->nconsumers; i++) {
        ledger_consumer_wait(&group->consumers[i]);
    }
}

void ledger_consumer_group_free(ledger_consumer_group *group) {
    free(group->consumers);
}
