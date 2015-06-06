#include "position_storage.h"

#define MAX_CONSUMERS 255

void ledger_position_storage_init(ledger_position_storage *storage) {
    dict_init(&storage->positions, MAX_CONSUMERS, (dict_comp_t)strcmp);
}

ledger_status ledger_position_storage_open(ledger_position_storage *storage) {
    return LEDGER_OK;
}

void ledger_position_storage_close(ledger_position_storage *storage) {
    dnode_t *cur;
    uint64_t *pos = NULL;

    for(cur = dict_first(&storage->positions);
        cur != NULL;
        cur = dict_next(&storage->positions, cur)) {

        pos = dnode_get(cur);
        free(pos);
    }

    dict_free_nodes(&storage->positions);
}

ledger_status ledger_position_storage_set(ledger_position_storage *storage,
                                          const char *position_key, uint64_t pos) {
    ledger_status rc;
    uint64_t *st_pos = NULL;
    dnode_t *dpos;

    dpos = dict_lookup(&storage->positions, position_key);
    if(dpos == NULL) {
        st_pos = malloc(sizeof(uint64_t));
        ledger_check_rc(st_pos != NULL, LEDGER_ERR_MEMORY, "Failed to allocate position");

        rc = dict_alloc_insert(&storage->positions, position_key, st_pos);
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
    return rc;
}

ledger_status ledger_position_storage_get(ledger_position_storage *storage,
                                          const char *position_key, uint64_t *pos) {
    uint64_t *st_pos = NULL;
    dnode_t *dpos;

    dpos = dict_lookup(&storage->positions, position_key);
    if(dpos == NULL) {
        return LEDGER_ERR_POSITION_NOT_FOUND;
    }
    st_pos = dnode_get(dpos);
    *pos = *st_pos;

    return LEDGER_OK;
}
