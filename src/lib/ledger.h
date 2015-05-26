#ifndef LIB_LEDGER_H
#define LIB_LEDGER_H

#include "common.h"
#include "dict.h"
#include "topic.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define LEDGER_BEGIN 0
#define LEDGER_CHUNK_SIZE 64

typedef struct {
    const char *root_directory;
    const char *last_error;
    dict_t topics;
} ledger_ctx;

const char *ledger_err(ledger_ctx *ctx);
ledger_status ledger_open_context(ledger_ctx *ctx, const char *root_directory);
ledger_status ledger_open_topic(ledger_ctx *ctx, const char *name,
                                unsigned int partition_count, ledger_topic_options *options);
ledger_status ledger_write_partition(ledger_ctx *ctx, const char *name,
                                     unsigned int partition_num, void *data,
                                     size_t len);
ledger_status ledger_read_partition(ledger_ctx *ctx, const char *name,
                                    unsigned int partition_num, uint64_t start_id,
                                    size_t nmessages, ledger_message_set *messages);
void ledger_close_context(ledger_ctx *ctx);

#if defined(__cplusplus)
}
#endif
#endif
