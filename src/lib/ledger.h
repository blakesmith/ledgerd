#ifndef LIB_LEDGER_H
#define LIB_LEDGER_H

#if defined(__cplusplus)
#include <cstddef>
#endif

#include "common.h"
#include "dict.h"
#include "position_storage.h"
#include "topic.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
    const char *root_directory;
    const char *last_error;
    dict_t topics;
    ledger_position_storage position_storage;
} ledger_ctx;

const char *ledger_err(ledger_ctx *ctx);
ledger_status ledger_open_context(ledger_ctx *ctx, const char *root_directory);
ledger_status ledger_open_topic(ledger_ctx *ctx, const char *name,
                                unsigned int partition_count, ledger_topic_options *options);
ledger_status ledger_write(ledger_ctx *ctx, const char *topic_name,
                           const char *partition_key, size_t key_len,
                           void *data, size_t len,
                           ledger_write_status *status);
ledger_status ledger_write_partition(ledger_ctx *ctx, const char *name,
                                     unsigned int partition_num, void *data,
                                     size_t len, ledger_write_status *status);
ledger_status ledger_read_partition(ledger_ctx *ctx, const char *name,
                                    unsigned int partition_num, uint64_t start_id,
                                    size_t nmessages, ledger_message_set *messages);
ledger_status ledger_wait_messages(ledger_ctx *ctx, const char *name,
                                   unsigned int partition_num);
ledger_status ledger_signal_readers(ledger_ctx *ctx, const char *name,
                                    unsigned int partition_num);

void ledger_close_context(ledger_ctx *ctx);


#if defined(__cplusplus)
}
#endif
#endif
