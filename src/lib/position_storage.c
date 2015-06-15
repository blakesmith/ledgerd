#include "position_storage.h"

#include <stdio.h>

#define N_BUCKETS 255
#define N_CELLS 5

static ssize_t make_key(const char *position_key, unsigned int partition_num,
                              char **key_out) {
    ledger_status rc;
    size_t key_len, total_len;
    char part_num[6];
    char *key_with_part = NULL;

    rc = snprintf(part_num, 6, "%05d", partition_num);
    ledger_check_rc(rc > 0, LEDGER_ERR_GENERAL, "Error building partition number");

    key_len = strlen(position_key);
    total_len = key_len+6;

    key_with_part = malloc(total_len);
    ledger_check_rc(key_with_part != NULL, LEDGER_ERR_MEMORY, "Error allocating key memory");

    memcpy(key_with_part, position_key, key_len);
    memcpy(key_with_part+key_len, part_num, 6);

    *key_out = key_with_part;

    return total_len;

error:
    if(key_with_part) {
        free(key_with_part);
    }
    return LEDGER_ERR_GENERAL;
}

void ledger_position_storage_init(ledger_position_storage *storage) {
    storage->position_path = NULL;
    fsd_map_init(&storage->positions, N_BUCKETS, N_CELLS);
}

ledger_status ledger_position_storage_open(ledger_position_storage *storage, const char *root_directory) {
    int rs;
    ledger_status rc;
    ssize_t size;
    char *position_path = NULL;

    size = ledger_concat_path(root_directory, "position_storage.map", &position_path);
    ledger_check_rc(size > 0, size, "Failed to build position storage map path");

    storage->position_path = position_path;

    rs = fsd_map_open(&storage->positions, storage->position_path);
    ledger_check_rc(rs == FSD_MAP_OK, LEDGER_ERR_GENERAL, "Failed to open position storage map");

    return LEDGER_OK;

error:
    if(position_path) {
        free(position_path);
    }
    return rc;
}

void ledger_position_storage_close(ledger_position_storage *storage) {
    fsd_map_close(&storage->positions);

    if(storage->position_path) {
        free(storage->position_path);
    }
}

ledger_status ledger_position_storage_set(ledger_position_storage *storage,
                                          const char *position_key, unsigned int partition_num,
                                          uint64_t pos) {
    ssize_t rc;
    char *key_with_part = NULL;

    rc = make_key(position_key, partition_num, &key_with_part);
    ledger_check_rc(rc > 0, rc, "Error building position storage key");

    rc = fsd_map_set(&storage->positions, (const char *)key_with_part,
                     rc, pos);
    ledger_check_rc(rc == FSD_MAP_OK, LEDGER_ERR_GENERAL, "Failed to set the position storage map location");

    free(key_with_part);
    return LEDGER_OK;

error:
    if(key_with_part) {
        free(key_with_part);
    }
    return rc;
}

ledger_status ledger_position_storage_get(ledger_position_storage *storage,
                                          const char *position_key, unsigned int partition_num,
                                          uint64_t *pos) {
    ssize_t rc;
    char *key_with_part = NULL;

    rc = make_key(position_key, partition_num, &key_with_part);
    ledger_check_rc(rc > 0, rc, "Error building position storage key");

    rc = fsd_map_get(&storage->positions, (const char *)key_with_part,
                     rc, pos);
    ledger_check_rc(rc == FSD_MAP_OK || rc == FSD_MAP_NOT_FOUND, LEDGER_ERR_GENERAL, "Failed to get the position storage map location");
    if(rc == FSD_MAP_NOT_FOUND) {
        return LEDGER_ERR_POSITION_NOT_FOUND;
    }

    free(key_with_part);
    return LEDGER_OK;

error:
    if(key_with_part) {
        free(key_with_part);
    }
    return rc;
}
