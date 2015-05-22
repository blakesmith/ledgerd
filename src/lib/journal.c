#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
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

    rc = snprintf(journal_path, 13, "%08d.%s", journal->id, JOURNAL_IDX_EXT);
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

    rc = snprintf(journal_path, 13, "%08d.%s", journal->id, JOURNAL_EXT);
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
                                  uint32_t id) {
    ledger_status rc;

    rc = open_journal(journal, partition_path, id);
    ledger_check_rc(rc == LEDGER_OK, rc, "Failed to open ledger journal");

    rc = open_journal_index(journal, partition_path, id);
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

