#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "dict.h"
#include "ledger.h"
#include "murmur3.h"
#include "position_storage.h"
#include "topic.h"

#define MAX_TOPICS 255

const char *ledger_err(ledger_ctx *ctx) {
    return ctx->last_error;
}

ledger_status ledger_open_context(ledger_ctx *ctx, const char *root_directory) {
    ledger_status rc;

    ctx->root_directory = root_directory;
    dict_init(&ctx->topics, MAX_TOPICS, (dict_comp_t)strcmp);

    ledger_position_storage_init(&ctx->position_storage);
    rc = ledger_position_storage_open(&ctx->position_storage, root_directory);
    ledger_check_rc(rc == LEDGER_OK, rc, "Failed to open position storage");

    return LEDGER_OK;

error:
    return rc;
}

void ledger_close_context(ledger_ctx *ctx) {
    dnode_t *cur;
    ledger_topic *topic = NULL;

    for(cur = dict_first(&ctx->topics);
        cur != NULL;
        cur = dict_next(&ctx->topics, cur)) {

        topic = dnode_get(cur);
        ledger_topic_close(topic);
        free(topic);
    }

    ledger_position_storage_close(&ctx->position_storage);
    dict_free_nodes(&ctx->topics);
}

ledger_status ledger_open_topic(ledger_ctx *ctx,
                                const char *name,
                                const unsigned int *partition_ids,
                                size_t partition_count,
                                ledger_topic_options *options) {
    ledger_status rc;
    int rv;
    ledger_topic *topic = NULL;
    ledger_topic *lookup = NULL;

    ledger_check_rc(partition_count > 0, LEDGER_ERR_BAD_TOPIC, "You must specify more than one partition");

    lookup = ledger_lookup_topic(ctx, name);
    ledger_check_rc(lookup == NULL, LEDGER_ERR_BAD_TOPIC, "That topic already exists");

    rc = ledger_topic_new(name, &topic);
    ledger_check_rc(rc == LEDGER_OK, LEDGER_ERR_MEMORY, "Failed to allocate topic");

    rv = dict_alloc_insert(&ctx->topics, topic->name, topic);
    ledger_check_rc(rv == 1, LEDGER_ERR_GENERAL, "Failed to insert topic into context");

    return ledger_topic_open(topic, ctx->root_directory,
                             partition_ids, partition_count,
                             options);

error:
    if(topic) {
        free(topic);
    }
    return rc;
}

ledger_topic *ledger_lookup_topic(ledger_ctx *ctx, const char *name) {
    ledger_topic *topic = NULL;
    dnode_t *dtopic;

    dtopic = dict_lookup(&ctx->topics, name);
    if(dtopic != NULL) {
        topic = dnode_get(dtopic);
        return topic;
    }
    return NULL;
}

ledger_status ledger_write_partition(ledger_ctx *ctx, const char *name,
                                     unsigned int partition_num, void *data,
                                     size_t len, ledger_write_status *status) {
    ledger_status rc;
    ledger_topic *topic = NULL;

    topic = ledger_lookup_topic(ctx, name);
    ledger_check_rc(topic != NULL, LEDGER_ERR_BAD_TOPIC, "Topic not found");

    return ledger_topic_write_partition(topic, partition_num, data, len, status);

error:
    return rc;
}

ledger_status ledger_write(ledger_ctx *ctx, const char *topic_name,
                           const char *partition_key, size_t key_len,
                           void *data, size_t len,
                           ledger_write_status *status) {
    uint32_t hash;
    unsigned int partition_num;
    ledger_status rc;
    ledger_topic *topic = NULL;
    static const uint32_t seed = 42;

    topic = ledger_lookup_topic(ctx, topic_name);
    ledger_check_rc(topic != NULL, LEDGER_ERR_BAD_TOPIC, "Topic not found");

    MurmurHash3_x86_32(partition_key, key_len, seed, &hash);
    partition_num = hash % topic->npartitions;

    return ledger_topic_write_partition(topic, partition_num, data, len, status);

error:
    return rc;
}

ledger_status ledger_latest_message_id(ledger_ctx *ctx, const char *name,
                                       unsigned int partition_num, uint64_t *id) {
    ledger_status rc;
    ledger_topic *topic = NULL;

    topic = ledger_lookup_topic(ctx, name);
    ledger_check_rc(topic != NULL, LEDGER_ERR_BAD_TOPIC, "Topic not found");

    return ledger_topic_latest_message_id(topic, partition_num, id);

error:
    return rc;
}

ledger_status ledger_read_partition(ledger_ctx *ctx, const char *name,
                                    unsigned int partition_num, uint64_t start_id,
                                    size_t nmessages, ledger_message_set *messages) {
    ledger_status rc;
    ledger_topic *topic = NULL;

    topic = ledger_lookup_topic(ctx, name);
    ledger_check_rc(topic != NULL, LEDGER_ERR_BAD_TOPIC, "Topic not found");

    return ledger_topic_read_partition(topic, partition_num, start_id,
                                       nmessages, messages);

error:
    return rc;
}

ledger_status ledger_wait_messages(ledger_ctx *ctx, const char *name,
                                   unsigned int partition_num) {
    ledger_status rc;
    ledger_topic *topic = NULL;

    topic = ledger_lookup_topic(ctx, name);
    ledger_check_rc(topic != NULL, LEDGER_ERR_BAD_TOPIC, "Topic not found");

    return ledger_topic_wait_messages(topic, partition_num);

error:
    return rc;
}

ledger_status ledger_signal_readers(ledger_ctx *ctx, const char *name,
                                    unsigned int partition_num) {
    ledger_status rc;
    ledger_topic *topic = NULL;

    topic = ledger_lookup_topic(ctx, name);
    ledger_check_rc(topic != NULL, LEDGER_ERR_BAD_TOPIC, "Topic not found");

    return ledger_topic_signal_readers(topic, partition_num);

error:
    return rc;
}

