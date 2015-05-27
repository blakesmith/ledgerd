#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>

#include "common.h"
#include "journal.h"
#include "partition.h"

#define META_FILE "meta"

static ledger_status remove_old_journals(ledger_partition *partition) {
    // TODO: implement
    return LEDGER_OK;
}

static inline ledger_journal_meta_entry *find_latest_meta(ledger_partition *partition) {
    return &partition->meta.entries[partition->meta.nentries-1];
}

static ledger_status find_latest_message_id(ledger_partition *partition, uint64_t *id) {
    ledger_journal_meta_entry *latest_meta = find_latest_meta(partition);
    ledger_journal journal;
    ledger_journal_options journal_options;
    ledger_status rc;

    rc = ledger_journal_open(&journal, partition->path, latest_meta, &journal_options);
    ledger_check_rc(rc == LEDGER_OK, rc, "Failed to open journal");

    rc = ledger_journal_latest_message_id(&journal, id);
    ledger_check_rc(rc == LEDGER_OK, rc, "Failed to find the latest message id");

    ledger_journal_close(&journal);
    return LEDGER_OK;

error:
    ledger_journal_close(&journal);
    return rc;
}

static ledger_status add_journal(ledger_partition *partition, int fd, bool initial) {
    ledger_status rc;
    pthread_mutexattr_t mattr;
    struct timeval tv;
    uint32_t nentries;
    off_t offset;
    ledger_journal_meta_entry meta_entry;

    rc = gettimeofday(&tv, NULL);
    ledger_check_rc(rc == 0, LEDGER_ERR_GENERAL, "Failed to fetch initial meta time of day");

    rc = pthread_mutexattr_init(&mattr);
    ledger_check_rc(rc == 0, LEDGER_ERR_GENERAL, "Failed to initialize mutex attribute");

    rc = pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    ledger_check_rc(rc == 0, LEDGER_ERR_GENERAL, "Failed to set mutex attribute to shared");

    rc = pthread_mutex_init(&meta_entry.journal_write_lock, &mattr);
    ledger_check_rc(rc == 0, LEDGER_ERR_GENERAL, "Failed to initialize index write mutex");

    if(initial) {
        nentries = 1;
        meta_entry.id = 0;
        meta_entry.first_journal_id = 0;
    } else {
        nentries = partition->meta.nentries + 1;
        meta_entry.id = partition->meta.nentries;
        rc = find_latest_message_id(partition, &meta_entry.first_journal_id);
        ledger_check_rc(rc == LEDGER_OK, rc, "Failed to fetch the latest message id");
    }

    meta_entry.first_journal_time = tv.tv_sec;

    rc = ledger_pwrite(fd, (void *)&nentries, sizeof(uint32_t), 0);
    ledger_check_rc(rc, LEDGER_ERR_IO, "Failed to write meta number of entries");

    offset = sizeof(uint32_t) + partition->meta.nentries * sizeof(meta_entry);
    rc = ledger_pwrite(fd, (void *)&meta_entry, sizeof(meta_entry), offset);
    ledger_check_rc(rc, LEDGER_ERR_IO, "Failed to write meta initial entry");

    partition->meta.nentries++;

    return LEDGER_OK;

error:
    return rc;
}

static ledger_journal_meta_entry *find_meta(ledger_partition *partition, uint64_t message_id) {
    ledger_journal_meta_entry *entry = NULL;
    int i;

    for(i = partition->meta.nentries-1; i >= 0; i--) {
        entry = &partition->meta.entries[i];
        if(entry->first_journal_id <= message_id) {
            return entry;
        }
    }

    return entry;
}

static void init_meta(ledger_partition_meta *meta) {
    meta->nentries = 0;
    meta->entries = NULL;
    meta->map = NULL;
    meta->map_len = 0;
}

static int open_meta(ledger_partition *partition) {
    int fd = 0;
    ledger_status rc;
    char *meta_path = NULL;
    ssize_t path_len;

    path_len = ledger_concat_path(partition->path, META_FILE, &meta_path);
    ledger_check_rc(path_len > 0, LEDGER_ERR_MEMORY, "Failed to build meta path");

    fd = open(meta_path, O_RDWR|O_CREAT, 0700);
    ledger_check_rc(fd > 0 || errno == EEXIST, LEDGER_ERR_BAD_META, "Failed to open meta file");

    free(meta_path);
    return fd;

error:
    if(meta_path) {
        free(meta_path);
    }
    if(fd) {
        close(fd);
    }
    return rc;
}

static ledger_status remap_meta(ledger_partition *partition, int fd, size_t map_len) {
    ledger_status rc;
    uint32_t *nentries;
    size_t expected_len, old_len;
    void *map, *old;

    ledger_check_rc(map_len >= sizeof(uint32_t),
                    LEDGER_ERR_BAD_META, "Corrupt meta file, should contain the number of meta entries");

    map = mmap(NULL, map_len, PROT_READ|PROT_WRITE,
               MAP_SHARED, fd, 0);
    ledger_check_rc(map != MAP_FAILED, LEDGER_ERR_IO, "Failed to memory map the meta file");

    nentries = (uint32_t *)map;

    expected_len = sizeof(uint32_t) + (*nentries * sizeof(ledger_journal_meta_entry));
    ledger_check_rc(expected_len == map_len, LEDGER_ERR_BAD_META, "Corrupt meta file, expected length does not match file length");

    partition->meta.nentries = *nentries;
    nentries++;
    partition->meta.entries = (ledger_journal_meta_entry *)nentries;

    if(partition->meta.map) {
        old = partition->meta.map;
        old_len = partition->meta.map_len;

        partition->meta.map = map;
        partition->meta.map_len = map_len;

        rc = munmap(old, old_len);
        ledger_check_rc(rc == 0, LEDGER_ERR_IO, "Failed to unmap old meta file");
    }

    return LEDGER_OK;

error:
    return rc;
}

static ledger_status unmap_meta(ledger_partition *partition) {
    if(partition->meta.map) {
        munmap(partition->meta.map, partition->meta.map_len);
    }
    return LEDGER_OK;
}

static ledger_status rotate_journals(ledger_partition *partition) {
    ledger_status rc;
    int fd = 0;
    struct stat st;

    rc = remove_old_journals(partition);
    ledger_check_rc(rc == LEDGER_OK, rc, "Failed to remove old journals");

    fd = open_meta(partition);
    ledger_check_rc(fd > 0, rc, "Failed to open meta file");

    rc = add_journal(partition, fd, false);
    ledger_check_rc(rc == LEDGER_OK, rc, "Failed to add a journal");

    rc = fstat(fd, &st);
    ledger_check_rc(rc == 0, LEDGER_ERR_IO, "Failed to stat meta file");

    rc = remap_meta(partition, fd, st.st_size);
    ledger_check_rc(rc == LEDGER_OK, rc, "Failed to remap meta during rotation");

    close(fd);
    return LEDGER_OK;

error:
    if(fd) {
        close(fd);
    }
    return rc;
}

ledger_status ledger_partition_open(ledger_partition *partition, const char *topic_path,
                                    unsigned int partition_number, ledger_partition_options *options) {
    ledger_status rc;
    int fd = 0;
    char part_num[5];
    ssize_t path_len;
    char *partition_path = NULL;
    struct stat st;

    partition->path = NULL;
    partition->opened = false;

    rc = snprintf(part_num, 5, "%d", partition_number);
    ledger_check_rc(rc > 0, LEDGER_ERR_GENERAL, "Error building partition dir part");

    path_len = ledger_concat_path(topic_path, part_num, &partition_path);
    ledger_check_rc(path_len > 0, path_len, "Failed to construct partition directory path");

    memcpy(&partition->options, options, sizeof(ledger_partition_options));
    partition->path = partition_path;
    partition->path_len = path_len;

    rc = mkdir(partition_path, 0755);
    ledger_check_rc(rc == 0 || errno == EEXIST, LEDGER_ERR_MKDIR, "Failed to create partition directory");

    init_meta(&partition->meta);

    fd = open_meta(partition);
    ledger_check_rc(fd > 0, rc, "Failed to open meta file");

    rc = fstat(fd, &st);
    ledger_check_rc(rc == 0, LEDGER_ERR_IO, "Failed to stat meta file");

    if(st.st_size == 0) {
        rc = add_journal(partition, fd, true);
        ledger_check_rc(rc == LEDGER_OK, rc, "Failed to add initial journal");

        rc = fstat(fd, &st);
        ledger_check_rc(rc == 0, LEDGER_ERR_IO, "Failed to restat meta file");
    }

    rc = remap_meta(partition, fd, st.st_size);
    ledger_check_rc(rc == LEDGER_OK, rc, "Failed to read memory mapped meta file");

    close(fd);

    partition->opened = true;

    return LEDGER_OK;

error:
    if(partition_path) {
        free(partition_path);
    }
    if(fd) {
        close(fd);
    }
    return rc;
}

ledger_status ledger_partition_write(ledger_partition *partition, void *data,
                                     size_t len) {
    ledger_status rc, write_status;
    ledger_journal_meta_entry *latest_meta = NULL;
    ledger_journal journal;
    ledger_journal_options journal_options;

    ledger_check_rc(partition->meta.nentries > 0, LEDGER_ERR_BAD_PARTITION, "No journal entry to write to");

    journal_options.drop_corrupt = partition->options.drop_corrupt;
    journal_options.max_size_bytes = partition->options.journal_max_size_bytes;

    do {
        latest_meta = find_latest_meta(partition);

        rc = pthread_mutex_lock(&latest_meta->journal_write_lock);
        ledger_check_rc(rc == 0, LEDGER_ERR_GENERAL, "Failed to lock partition for writing");

        rc = ledger_journal_open(&journal, partition->path, latest_meta, &journal_options);
        ledger_check_rc(rc == LEDGER_OK, rc, "Failed to open journal");

        rc = ledger_journal_write(&journal, data, len);
        ledger_check_rc(rc == LEDGER_OK || rc == LEDGER_NEXT, rc, "Failed to write to journal");

        write_status = rc;
        if(write_status == LEDGER_NEXT) {
            rc = rotate_journals(partition);
            ledger_check_rc(rc == LEDGER_OK, rc, "Failed to rotate journals");
        }

        rc = pthread_mutex_unlock(&latest_meta->journal_write_lock);
        ledger_check_rc(rc == 0, LEDGER_ERR_GENERAL, "Failed to unlock partition for writing");

        ledger_journal_close(&journal);
    } while (write_status == LEDGER_NEXT);
    return LEDGER_OK;

error:
    pthread_mutex_unlock(&latest_meta->journal_write_lock);
    ledger_journal_close(&journal);
    return rc;
}

ledger_status ledger_partition_read(ledger_partition *partition, uint64_t start_id,
                                    size_t nmessages, ledger_message_set *messages) {
    ledger_status rc;
    ledger_journal_meta_entry *meta;
    ledger_journal journal;
    ledger_journal_options journal_options;
    uint64_t message_id;
    size_t messages_left;

    ledger_check_rc(partition->meta.nentries > 0, LEDGER_ERR_BAD_PARTITION, "No journal entry to read from");

    journal_options.drop_corrupt = partition->options.drop_corrupt;
    journal_options.max_size_bytes = partition->options.journal_max_size_bytes;

    memset(messages, 0, sizeof(ledger_message_set));
    message_id = start_id;
    messages_left = nmessages;
    do {
        meta = find_meta(partition, message_id);

        rc = ledger_journal_open(&journal, partition->path, meta, &journal_options);
        ledger_check_rc(rc == LEDGER_OK, rc, "Failed to open journal");

        rc = ledger_journal_read(&journal, message_id, messages_left, messages);
        ledger_check_rc(rc == LEDGER_OK || rc == LEDGER_NEXT, rc, "Failed to read from the journal");

        ledger_journal_close(&journal);

        messages_left = messages_left - messages->nmessages;
        message_id = messages->next_id;

        if(meta->id == partition->meta.nentries-1) {
            // Reached the last journal, and there's no more messages to read
            break;
        }
    } while (rc == LEDGER_NEXT);

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
    unmap_meta(partition);
    partition->opened = false;
}

