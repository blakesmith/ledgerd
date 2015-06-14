#include "position_storage.h"

#include <stdio.h>

#define MAX_CONSUMERS 255

static ledger_status make_key(const char *position_key, unsigned int partition_num,
                              char **key_out) {
    ledger_status rc;
    size_t key_len;
    char part_num[6];
    char *key_with_part = NULL;

    rc = snprintf(part_num, 6, "%05d", partition_num);
    ledger_check_rc(rc > 0, LEDGER_ERR_GENERAL, "Error building partition number");

    key_len = strlen(position_key);

    key_with_part = malloc(key_len+6);
    ledger_check_rc(key_with_part != NULL, LEDGER_ERR_MEMORY, "Error allocating key memory");

    memcpy(key_with_part, position_key, key_len);
    memcpy(key_with_part+key_len, part_num, 6);

    *key_out = key_with_part;

    return LEDGER_OK;

error:
    if(key_with_part) {
        free(key_with_part);
    }
    return LEDGER_ERR_GENERAL;
}

void ledger_position_storage_init(ledger_position_storage *storage) {
    storage->position_path = NULL;
    dict_init(&storage->positions, MAX_CONSUMERS, (dict_comp_t)strcmp);
}

ledger_status ledger_position_storage_open(ledger_position_storage *storage, const char *root_directory) {
    ledger_status rc;
    ssize_t size;
    char *position_path = NULL;

    size = ledger_concat_path(root_directory, "position_storage.map", &position_path);
    ledger_check_rc(size > 0, size, "Failed to build position storage map path");

    storage->position_path = position_path;
    return LEDGER_OK;

error:
    if(position_path) {
        free(position_path);
    }
    return rc;
}

void ledger_position_storage_close(ledger_position_storage *storage) {
    dnode_t *cur;
    uint64_t *pos = NULL;
    const char *key = NULL;

    for(cur = dict_first(&storage->positions);
        cur != NULL;
        cur = dict_next(&storage->positions, cur)) {

        pos = dnode_get(cur);
        key = dnode_getkey(cur);
        free(pos);
        free((char *)key);
    }

    dict_free_nodes(&storage->positions);

    if(storage->position_path) {
        free(storage->position_path);
    }
}

ledger_status ledger_position_storage_set(ledger_position_storage *storage,
                                          const char *position_key, unsigned int partition_num,
                                          uint64_t pos) {
    ledger_status rc;
    uint64_t *st_pos = NULL;
    dnode_t *dpos;
    char *key_with_part = NULL;

    rc = make_key(position_key, partition_num, &key_with_part);
    ledger_check_rc(rc == LEDGER_OK, rc, "Error building position storage key");

    dpos = dict_lookup(&storage->positions, key_with_part);
    if(dpos == NULL) {
        st_pos = malloc(sizeof(uint64_t));
        ledger_check_rc(st_pos != NULL, LEDGER_ERR_MEMORY, "Failed to allocate position");

        rc = dict_alloc_insert(&storage->positions, key_with_part, st_pos);
        ledger_check_rc(rc == 1, LEDGER_ERR_GENERAL, "Failed to insert position into storage");
    } else {
        st_pos = dnode_get(dpos);
    }
    *st_pos = pos;

    return LEDGER_OK;

error:
    if(st_pos) {
        free(st_pos);
    }
    if(key_with_part) {
        free(key_with_part);
    }
    return rc;
}

ledger_status ledger_position_storage_get(ledger_position_storage *storage,
                                          const char *position_key, unsigned int partition_num,
                                          uint64_t *pos) {
    ledger_status rc;
    uint64_t *st_pos = NULL;
    dnode_t *dpos;
    char *key_with_part = NULL;

    rc = make_key(position_key, partition_num, &key_with_part);
    ledger_check_rc(rc == LEDGER_OK, rc, "Error building position storage key");

    dpos = dict_lookup(&storage->positions, key_with_part);
    if(dpos == NULL) {
        free(key_with_part);
        return LEDGER_ERR_POSITION_NOT_FOUND;
    }
    st_pos = dnode_get(dpos);
    *pos = *st_pos;

    free(key_with_part);
    return LEDGER_OK;

error:
    if(key_with_part) {
        free(key_with_part);
    }
    return rc;
}
