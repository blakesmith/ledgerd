#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "crc32.h"
#include "journal.h"

#define JOURNAL_EXT "jnl"
#define JOURNAL_IDX_EXT "idx"

ledger_status open_journal_index(ledger_journal *journal, const char *partition_path,
                                 uint32_t id) {
    ledger_status rc;
    char *path = NULL;
    char journal_path[13];
    size_t path_len;
    
    journal->idx.fd = -1;

    rc = snprintf(journal_path, 13, "%08d.%s", journal->metadata->id, JOURNAL_IDX_EXT);
    ledger_check_rc(rc > 0, LEDGER_ERR_GENERAL, "Error building journal index path");

    path_len = ledger_concat_path(partition_path, journal_path, &path);
    ledger_check_rc(path_len > 0, LEDGER_ERR_MEMORY, "Failed to build journal index path");

    journal->idx.fd = open(path, O_RDWR|O_CREAT|O_APPEND, 0700);
    ledger_check_rc(journal->idx.fd > 0 || errno == EEXIST, LEDGER_ERR_IO, "Failed to open journal index file");

    free(path);
    return LEDGER_OK;

error:
    if(journal->idx.fd > 0) {
        close(journal->idx.fd);
    }
    if(path) {
        free(path);
    }
    return rc;
}

ledger_status open_journal(ledger_journal *journal, const char *partition_path,
                           uint32_t id) {
    ledger_status rc;
    char *path = NULL;
    char journal_path[13];
    size_t path_len;

    journal->fd = -1;

    rc = snprintf(journal_path, 13, "%08d.%s", journal->metadata->id, JOURNAL_EXT);
    ledger_check_rc(rc > 0, LEDGER_ERR_GENERAL, "Error building journal path");

    path_len = ledger_concat_path(partition_path, journal_path, &path);
    ledger_check_rc(path_len > 0, LEDGER_ERR_MEMORY, "Failed to build journal path");

    journal->fd = open(path, O_RDWR|O_CREAT, 0700);
    ledger_check_rc(journal->fd > 0 || errno == EEXIST, LEDGER_ERR_IO, "Failed to open journal file");

    free(path);
    return LEDGER_OK;

error:
    if(journal->fd > 0) {
        close(journal->fd);
    }
    if(path) {
        free(path);
    }
    return rc;
}

ledger_status ledger_journal_open(ledger_journal *journal, const char *partition_path,
                                  ledger_journal_meta_entry *metadata) {
    ledger_status rc;

    journal->metadata = metadata;

    rc = open_journal(journal, partition_path, metadata->id);
    ledger_check_rc(rc == LEDGER_OK, rc, "Failed to open ledger journal");

    rc = open_journal_index(journal, partition_path, metadata->id);
    ledger_check_rc(rc == LEDGER_OK, rc, "Failed to open ledger journal index");    

    return LEDGER_OK;

error:
    return rc;
}
 

void ledger_journal_close(ledger_journal *journal) {
    if(journal->fd) {
        close(journal->fd);
    }
    if(journal->idx.fd) {
        close(journal->idx.fd);
    }
}

ledger_status ledger_journal_write(ledger_journal *journal, void *data,
                                   size_t len) {
    ledger_status rc;
    ledger_message_hdr message_header;
    uint64_t offset;
    struct stat st;

    message_header.len = len;
    message_header.crc32 = crc32_compute(0, data, len);

    rc = fstat(journal->fd, &st);
    ledger_check_rc(rc == 0, LEDGER_ERR_IO, "Failed to stat journal file");

    rc = ledger_pwrite(journal->fd, (void *)&message_header, sizeof(message_header), st.st_size);
    ledger_check_rc(rc, LEDGER_ERR_IO, "Failed to write message header");

    rc = ledger_pwrite(journal->fd, data, len, st.st_size + sizeof(message_header));
    ledger_check_rc(rc, LEDGER_ERR_IO, "Failed to write message");

    offset = st.st_size;
    rc = ledger_pwrite(journal->idx.fd, (void *)&offset, sizeof(uint64_t), 0);
    ledger_check_rc(rc, LEDGER_ERR_IO, "Failed to write index offset");

    return LEDGER_OK;

error:
    return rc;
}

ledger_status ledger_journal_read(ledger_journal *journal, uint64_t last_id,
                                  size_t nmessages, ledger_message_set *messages) {
    ledger_status rc;
    struct stat idx_st, journal_st;
    size_t read_len;
    size_t total_messages;
    uint64_t first_journal_id;
    uint64_t start_idx_offset, end_idx_offset;
    int i;
    uint64_t message_offset;
    uint64_t *message_offsets;
    uint8_t *byte_map;
    ledger_message_hdr message_hdr;

    journal->idx.map = NULL;
    journal->map = NULL;

    rc = fstat(journal->idx.fd, &idx_st);
    ledger_check_rc(rc == 0, LEDGER_ERR_IO, "Failed to stat journal index file");

    rc = fstat(journal->fd, &journal_st);
    ledger_check_rc(rc == 0, LEDGER_ERR_IO, "Failed to stat journal file");

    first_journal_id = journal->metadata->first_journal_id;
    start_idx_offset = (last_id - first_journal_id) * sizeof(uint64_t);
    if(start_idx_offset > idx_st.st_size) {
        // That message doesn't exist yet?
        return LEDGER_OK;
    }
    end_idx_offset = start_idx_offset + nmessages;
    if(end_idx_offset > idx_st.st_size) {
        end_idx_offset = idx_st.st_size;
    }
    read_len = end_idx_offset - start_idx_offset;
    total_messages = read_len / sizeof(uint64_t);

    journal->idx.map = mmap(NULL, read_len, PROT_READ, MAP_PRIVATE,
                            journal->idx.fd, start_idx_offset);
    ledger_check_rc(journal->idx.map != (void *)-1, LEDGER_ERR_IO, "Failed to memory map journal index");
    journal->idx.map_len = read_len;

    journal->map = mmap(NULL, journal_st.st_size, PROT_READ, MAP_PRIVATE,
                        journal->fd, 0);
    ledger_check_rc(journal->map != (void *)-1, LEDGER_ERR_IO, "Failed to memory map journal");
    journal->map_len = journal_st.st_size;

    rc = ledger_message_set_init(messages, total_messages);
    ledger_check_rc(rc == LEDGER_OK, rc, "Failed to allocate message set");
    message_offsets = (uint64_t *)journal->idx.map;
    byte_map = (uint8_t *)journal->map;
    for(i = 0; i < total_messages; i++) {
        message_offset = *message_offsets;
        message_hdr.len = *((uint32_t *)(byte_map + message_offset));
        message_hdr.crc32 = *((uint32_t *)(byte_map + message_offset + sizeof(uint32_t)));
        messages->messages[i].len = message_hdr.len;
        messages->messages[i].data = (void *)(byte_map + message_offset + 2 * sizeof(uint32_t));
        message_offsets++;
    }

    munmap(journal->idx.map, journal->idx.map_len);
//    munmap(journal->map, journal->map_len);
    return LEDGER_OK;

error:
    if(journal->idx.map) {
        munmap(journal->idx.map, journal->idx.map_len);
    }
    if(journal->map) {
        munmap(journal->map, journal->map_len);
    }
    return rc;
}
