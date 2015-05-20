#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "dict.h"
#include "ledger.h"
#include "topic.h"

#define MAX_TOPICS 255

const char *ledger_err(ledger_ctx *ctx) {
    return ctx->last_error;
}

ledger_status ledger_open_context(ledger_ctx *ctx, const char *root_directory) {
    ctx->root_directory = root_directory;
    dict_init(&ctx->topics, MAX_TOPICS, (dict_comp_t)strcmp);
    return LEDGER_OK;
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

    dict_free_nodes(&ctx->topics);
}

ledger_status ledger_open_topic(ledger_ctx *ctx, const char *name,
                                unsigned int partition_count, int options) {
    ledger_status rc;
    int rv;
    ledger_topic *topic = malloc(sizeof(ledger_topic));
    ledger_check_rc(topic != NULL, LEDGER_ERR_MEMORY, "Failed to allocate topic");

    topic->name = name;

    rv = dict_alloc_insert(&ctx->topics, name, topic);
    ledger_check_rc(rv == 1, LEDGER_ERR_GENERAL, "Failed to insert topic into context");

    return ledger_topic_open(topic, ctx->root_directory,
                             name, partition_count, options);

error:
    return rc;
}

ledger_status ledger_write_partition(ledger_ctx *ctx, const char *name,
                                     unsigned int partition_num, void *data,
                                     size_t len) {
    ledger_status rc;
    ledger_topic *topic = NULL;
    dnode_t *dtopic;

    dtopic = dict_lookup(&ctx->topics, name);
    ledger_check_rc(dtopic != NULL, LEDGER_ERR_BAD_TOPIC, "Topic not found");

    topic = dnode_get(dtopic);
    return ledger_topic_write_partition(topic, partition_num, data, len);

error:
    return rc;
}

ledger_status ledger_read_partition(ledger_ctx *ctx, const char *topic,
                                    unsigned int partition_num, uint64_t last_id,
                                    size_t nmessages, ledger_message_set *messages) {
    return LEDGER_OK;
}
