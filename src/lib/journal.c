#define _XOPEN_SOURCE 500
#define __STDC_FORMAT_MACROS

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <inttypes.h>

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
                                  ledger_journal_meta_entry *metadata, ledger_journal_options *options) {
    ledger_status rc;

    journal->metadata = metadata;
    memcpy(&journal->options, options, sizeof(ledger_journal_options));

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

    if(st.st_size > journal->options.max_size_bytes) {
        return LEDGER_NEXT;
    }

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

ledger_status ledger_journal_latest_message_id(ledger_journal *journal, uint64_t *id) {
    ledger_status rc;
    uint64_t journal_id;
    struct stat idx_st;

    rc = fstat(journal->idx.fd, &idx_st);
    ledger_check_rc(rc == 0, LEDGER_ERR_IO, "Failed to stat journal index file");

    journal_id = idx_st.st_size / sizeof(uint64_t);
    *id = journal->metadata->first_message_id + journal_id;

    return LEDGER_OK;
error:
    return rc;
}

ledger_status ledger_journal_read(ledger_journal *journal, uint64_t start_id,
                                  size_t nmessages, ledger_message_set *messages) {
    ledger_status rc;
    int i;
    int ncorrupt = 0;
    struct stat idx_st;
    size_t journal_read_len, previous_count, total_messages;
    uint64_t first_message_id, index_id;
    uint64_t start_idx_offset, end_idx_offset;
    uint64_t message_offset;
    uint64_t *message_offsets;
    uint32_t crc32_verification;
    ledger_message_hdr message_hdr;
    ledger_message *current_message;
    bool over_journal = false;

    journal->idx.map = NULL;

    rc = fstat(journal->idx.fd, &idx_st);
    ledger_check_rc(rc == 0, LEDGER_ERR_IO, "Failed to stat journal index file");

    first_message_id = journal->metadata->first_message_id;
    index_id = start_id - first_message_id;
    start_idx_offset = index_id * sizeof(uint64_t);
    if(start_idx_offset > idx_st.st_size) {
        // Reached the end of the journal
        return LEDGER_NEXT;
    }
    end_idx_offset = start_idx_offset + (nmessages * sizeof(uint64_t));
    if(end_idx_offset > idx_st.st_size) {
        end_idx_offset = idx_st.st_size;
        over_journal = true;
    }
    journal_read_len = end_idx_offset - start_idx_offset;
    total_messages = journal_read_len / sizeof(uint64_t);

    /* printf("Start offset: %" PRIu64 ", End offset: %" PRIu64 */
    /*        ", Read length: %ld, File size: %ld\n", start_idx_offset, */
    /*        end_idx_offset, journal_read_len, idx_st.st_size); */
    journal->idx.map = mmap(NULL, idx_st.st_size, PROT_READ, MAP_PRIVATE,
                            journal->idx.fd, 0);
    ledger_check_rc(journal->idx.map != (void *)-1, LEDGER_ERR_IO, "Failed to memory map journal index");
    journal->idx.map_len = journal_read_len;

    if(messages->initialized) {
        previous_count = messages->nmessages;
        rc = ledger_message_set_grow(messages, total_messages);
        ledger_check_rc(rc == LEDGER_OK, rc, "Failed to grow message set");
    } else {
        previous_count = 0;
        rc = ledger_message_set_init(messages, total_messages);
        ledger_check_rc(rc == LEDGER_OK, rc, "Failed to allocate message set");
    }

    message_offsets = (uint64_t *)journal->idx.map;
    message_offsets = message_offsets + index_id;
    for(i = 0; i < total_messages; i++) {
        message_offset = *message_offsets;
        current_message = &messages->messages[i+previous_count-ncorrupt];

        rc = ledger_pread(journal->fd, (void *)&message_hdr,
                          sizeof(message_hdr), message_offset);
        ledger_check_rc(rc, LEDGER_ERR_IO, "Failed to read message header");

        current_message->data = malloc(message_hdr.len);
        ledger_check_rc(current_message->data != NULL, LEDGER_ERR_MEMORY, "Failed to allocate message buffer");

        current_message->len = message_hdr.len;

        rc = ledger_pread(journal->fd, current_message->data,
                          current_message->len, message_offset + sizeof(ledger_message_hdr));
        ledger_check_rc(rc, LEDGER_ERR_IO, "Failed to read message");

        if(journal->options.drop_corrupt) {
            crc32_verification = crc32_compute(0, current_message->data, current_message->len);
            if(crc32_verification != message_hdr.crc32) {
                // Message is corrupt
                ledger_message_free(current_message);
                messages->nmessages--;
                ncorrupt++;
            }
        }

        current_message->id = start_id + i;
        messages->next_id = start_id + i + 1;
        message_offsets++;
    }

    munmap(journal->idx.map, journal->idx.map_len);

    if(over_journal) {
        return LEDGER_NEXT;
    }

    return LEDGER_OK;

error:
    if(journal->idx.map) {
        munmap(journal->idx.map, journal->idx.map_len);
    }
    return rc;
}
