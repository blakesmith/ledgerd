#ifndef LIB_LEDGER_POSITION_STORAGE_H
#define LIB_LEDGER_POSITION_STORAGE_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "fixed_size_disk_map.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
    fsd_map_t positions;
    char *position_path;
} ledger_position_storage;

void ledger_position_storage_init(ledger_position_storage *storage);
ledger_status ledger_position_storage_open(ledger_position_storage *storage, const char *position_path);
void ledger_position_storage_close(ledger_position_storage *storage);

ledger_status ledger_position_storage_set(ledger_position_storage *storage,
                                          const char *position_key, unsigned int partition_num,
                                          uint64_t pos);
ledger_status ledger_position_storage_get(ledger_position_storage *storage,
                                          const char *position_key, unsigned int partition_num,
                                          uint64_t *pos);

#if defined(__cplusplus)
}
#endif
#endif
