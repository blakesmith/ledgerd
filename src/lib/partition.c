#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>

#include "common.h"
#include "journal.h"
#include "partition.h"

#define META_FILE "meta"

ledger_status open_meta(ledger_partition *partition) {
    int fd = 0;
    ledger_status rc;
    char *meta_path = NULL;
    off_t meta_size;
    ssize_t path_len;
    struct stat st;
    uint32_t nentries;
    ledger_journal_meta_entry meta_entry;
    struct timeval tv;
    pthread_mutexattr_t mattr;

    partition->meta.nentries = 0;
    partition->meta.entries = NULL;

    path_len = ledger_concat_path(partition->path, META_FILE, &meta_path);
    ledger_check_rc(path_len > 0, LEDGER_ERR_MEMORY, "Failed to build meta path");

    fd = open(meta_path, O_RDWR|O_CREAT, 0700);
    ledger_check_rc(fd > 0 || errno == EEXIST, LEDGER_ERR_BAD_META, "Failed to open meta file");

    rc = fstat(fd, &st);
    ledger_check_rc(rc == 0, LEDGER_ERR_IO, "Failed to stat meta file");
    meta_size = st.st_size;

    if(meta_size == 0) {
        rc = gettimeofday(&tv, NULL);
        ledger_check_rc(rc == 0, LEDGER_ERR_GENERAL, "Failed to fetch initial meta time of day");

        rc = pthread_mutexattr_init(&mattr);
        ledger_check_rc(rc == 0, LEDGER_ERR_GENERAL, "Failed to initialize mutex attribute");

        rc = pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
        ledger_check_rc(rc == 0, LEDGER_ERR_GENERAL, "Failed to set mutex attribute to shared");

        rc = pthread_mutex_init(&meta_entry.partition_write_lock, &mattr);
        ledger_check_rc(rc == 0, LEDGER_ERR_GENERAL, "Failed to initialize index write mutex");

        nentries = 1;
        meta_entry.id = 0;
        meta_entry.first_journal_id = 0;
        meta_entry.first_journal_time = tv.tv_sec;

        rc = ledger_pwrite(fd, (void *)&nentries, sizeof(uint32_t), 0);
        ledger_check_rc(rc, LEDGER_ERR_IO, "Failed to write meta number of entries");

        rc = ledger_pwrite(fd, (void *)&meta_entry, sizeof(meta_entry), sizeof(uint32_t));
        ledger_check_rc(rc, LEDGER_ERR_IO, "Failed to write meta initial entry");

        meta_size = sizeof(uint32_t) + sizeof(meta_entry);
    }

    partition->meta.map = mmap(NULL, meta_size, PROT_READ|PROT_WRITE,
                               MAP_SHARED, fd, 0);
    ledger_check_rc(partition->meta.map != (void *)-1, LEDGER_ERR_IO, "Failed to memory map the meta file");

    partition->meta.map_len = meta_size;
    partition->meta.opened = true;

    free(meta_path);
    close(fd);

    return LEDGER_OK;
    
error:
    if(meta_path) {
        free(meta_path);
    }
    if(fd) {
        close(fd);
    }
    return rc;
}

ledger_status remap_meta(ledger_partition *partition) {
    ledger_status rc;
    uint32_t *nentries;
    size_t expected_len;

    ledger_check_rc(partition->meta.map_len >= sizeof(uint32_t),
                    LEDGER_ERR_BAD_META, "Corrupt meta file, should contain the number of meta entries");

    nentries = (uint32_t *)partition->meta.map;

    expected_len = sizeof(uint32_t) + (*nentries * sizeof(ledger_journal_meta_entry));
    ledger_check_rc(partition->meta.map_len == expected_len,
                    LEDGER_ERR_BAD_META,
                    "Corrupt meta file. The size of the file does not match the expected number of entries");

    partition->meta.nentries = *nentries;
    nentries++;
    partition->meta.entries = (ledger_journal_meta_entry *)nentries;

    return LEDGER_OK;

error:
    return rc;
}

ledger_status close_meta(ledger_partition *partition) {
    if(partition->meta.opened) {
        munmap(partition->meta.map, partition->meta.map_len);
        partition->meta.opened = false;
        partition->meta.nentries = 0;
        partition->meta.entries = NULL;
    }
    return LEDGER_OK;
}

ledger_status ledger_partition_open(ledger_partition *partition, const char *topic_path,
                                    unsigned int partition_number) {
    ledger_status rc;
    char part_num[5];
    ssize_t path_len;
    char *partition_path = NULL;

    partition->path = NULL;
    partition->opened = false;

    rc = snprintf(part_num, 5, "%d", partition_number);
    ledger_check_rc(rc > 0, LEDGER_ERR_GENERAL, "Error building partition dir part");

    path_len = ledger_concat_path(topic_path, part_num, &partition_path);
    ledger_check_rc(path_len > 0, path_len, "Failed to construct partition directory path");

    partition->path = partition_path;
    partition->path_len = path_len;

    rc = mkdir(partition_path, 0755);
    ledger_check_rc(rc == 0 || errno == EEXIST, LEDGER_ERR_MKDIR, "Failed to create partition directory");

    rc = open_meta(partition);
    ledger_check_rc(rc == LEDGER_OK, rc, "Failed to open meta file");

    rc = remap_meta(partition);
    ledger_check_rc(rc == LEDGER_OK, rc, "Failed to read memory mapped meta file");

    partition->opened = true;

    return LEDGER_OK;

error:
    if(partition_path) {
        free(partition_path);
    }
    if(partition->meta.opened) {
        close_meta(partition);
    }
    return rc;
}

ledger_status ledger_partition_write(ledger_partition *partition, void *data,
                                     size_t len) {
    ledger_status rc;
    ledger_journal_meta_entry *latest_meta;
    ledger_journal journal;

    ledger_check_rc(partition->meta.nentries > 0, LEDGER_ERR_BAD_PARTITION, "No journal entry to write to");

    latest_meta = partition->meta.entries + partition->meta.nentries - 1;

    rc = pthread_mutex_lock(&latest_meta->partition_write_lock);
    ledger_check_rc(rc == 0, LEDGER_ERR_GENERAL, "Failed to lock partition for writing");

    rc = ledger_journal_open(&journal, partition->path, latest_meta);
    ledger_check_rc(rc == LEDGER_OK, rc, "Failed to open journal");

    rc = ledger_journal_write(&journal, data, len);
    ledger_check_rc(rc == LEDGER_OK, rc, "Failed to write to journal");

    rc = pthread_mutex_unlock(&latest_meta->partition_write_lock);
    ledger_check_rc(rc == 0, LEDGER_ERR_GENERAL, "Failed to unlock partition for writing");

    ledger_journal_close(&journal);
    return LEDGER_OK;

error:
    pthread_mutex_unlock(&latest_meta->partition_write_lock);
    ledger_journal_close(&journal);
    return rc;
}

ledger_status ledger_partition_read(ledger_partition *partition, uint64_t start_id,
                                    size_t nmessages, bool drop_corrupt, ledger_message_set *messages) {
    ledger_status rc;
    ledger_journal_meta_entry *latest_meta;
    ledger_journal journal;

    ledger_check_rc(partition->meta.nentries > 0, LEDGER_ERR_BAD_PARTITION, "No journal entry to read from");

    //XXX: Fix
    latest_meta = partition->meta.entries + partition->meta.nentries - 1;
    rc = ledger_journal_open(&journal, partition->path, latest_meta);
    ledger_check_rc(rc == LEDGER_OK, rc, "Failed to open journal");

    rc = ledger_journal_read(&journal, start_id, nmessages, drop_corrupt, messages);
    ledger_check_rc(rc == LEDGER_OK, rc, "Failed to read from the journal");

    ledger_journal_close(&journal);
    return LEDGER_OK;

error:
    ledger_journal_close(&journal);
    return rc;
}

void ledger_partition_close(ledger_partition *partition) {
    if(partition->opened) {
        if(partition->path) {
            free(partition->path);
        }
    }
    partition->opened = false;
}

