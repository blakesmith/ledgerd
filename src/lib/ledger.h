#ifndef LIB_LEDGER_H
#define LIB_LEDGER_H

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum {
    LEDGER_OK = 0,
    LEDGER_ERR_GENERAL = -1,
    LEDGER_ERR_MEMORY = -2,
    LEDGER_ERR_MKDIR = -3,
    LEDGER_ERR_ARGS = -4
} ledger_status;

typedef struct {
    uint32_t length;
    uint32_t crc32;
} ledger_message_hdr;

typedef struct {
    uint64_t first_message_id;
    uint64_t first_message_time;
} ledger_partition_index_hdr;

typedef struct {
    ledger_partition_index_hdr hdr;
    int fd;
} ledger_partition_index;

typedef struct {
    ledger_partition_index idx;
} ledger_partition;
  
typedef struct {
    const char *root_directory;
    const char *last_error;
} ledger_ctx;

const char *ledger_err(ledger_ctx *ctx);
ledger_status ledger_open_context(ledger_ctx *ctx, const char *root_directory);
ledger_status ledger_open_topic(ledger_ctx *ctx, const char *topic,
                                unsigned int partition_count, int options);
ledger_status ledger_write_partition(ledger_ctx *ctx, const char *topic,
                                     unsigned int partition_num, void *data,
                                     size_t len);
void ledger_close_context(ledger_ctx *ctx);

#if defined(__cplusplus)
}
#endif
#endif
