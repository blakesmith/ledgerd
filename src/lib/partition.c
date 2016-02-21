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
#define LOCK_FILE "locks"

static ledger_status purge_journals(int truncate_index) {
    // TODO: Remove journal files
    if(truncate_index == -1) {
        // No journals to be removed
        return LEDGER_OK;
    }

    return LEDGER_OK;
}

static ledger_status shrink_meta(int truncate_index,
                                 int meta_fd,
                                 int* new_meta_fd) {
    if(truncate_index == -1) {
        // No changes to be made to the meta table
        return LEDGER_OK;
    }

    // 1. Open tempfile for new rewritten meta file
    // 2. Calculate new number of entries. Truncate index math?
    // 3. Copy new entries from old file
    // 4. Close old meta file
    // 5. Rename tempfile -> current file
    // 6. Pass back pointer to new file
    // TODO: Truncate (rebuild?) meta table
    return LEDGER_OK;
}

// We pass in a new_meta_fd out pointer, which will always get a reference
// to the active file descriptor for the meta file. This function may or
// may not rewrite the meta file, in cases of truncation. After calling this
// function, it is not valid for callers to access meta_fd directly anymore.
static ledger_status remove_old_journals(ledger_partition *partition,
                                         int meta_fd,
                                         int *new_meta_fd) {
    int rc;
    struct timeval tv;
    time_t journal_age;
    ledger_journal_meta_entry *journal_meta;

    *new_meta_fd = meta_fd;
    if(partition->options.journal_purge_age_seconds == LEDGER_JOURNAL_NO_PURGE) {
        return LEDGER_OK;
    }

    rc = gettimeofday(&tv, NULL);
    ledger_check_rc(rc == 0, LEDGER_ERR_GENERAL, "Failed to fetch initial journal rotation time of day");

    // Truncate all journals up until this index, inclusive.
    int truncate_idx = -1;
    for(int i = 0; i < partition->meta.nentries; i++) {
        journal_meta = &partition->meta.entries[i];
        journal_age = tv.tv_sec - journal_meta->create_time;
        if(journal_age > partition->options.journal_purge_age_seconds) {
            truncate_idx = i;
        } else {
            break;
        }
    }

    rc = shrink_meta(truncate_idx, meta_fd, new_meta_fd);
    ledger_check_rc(rc == LEDGER_OK, LEDGER_ERR_GENERAL, "Failed to shrink meta file");

    return purge_journals(truncate_idx);

error:
    return rc;
}

static inline ledger_journal_meta_entry *find_latest_meta(ledger_partition *partition) {
    if(partition->meta.nentries == 0) {
        return NULL;
    }

    return &partition->meta.entries[partition->meta.nentries-1];
}

static ledger_status add_journal(ledger_partition *partition, int fd) {
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

    rc = pthread_mutex_init(&meta_entry.write_lock, &mattr);
    ledger_check_rc(rc == 0, LEDGER_ERR_GENERAL, "Failed to initialize index write mutex");

    rc = ledger_partition_latest_message_id(partition, &meta_entry.first_message_id);
    ledger_check_rc(rc == LEDGER_OK, rc, "Failed to fetch the latest message id");

    nentries = partition->meta.nentries + 1;
    meta_entry.id = partition->meta.nentries;

    meta_entry.create_time = tv.tv_sec;

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

static ledger_status open_lockfile(ledger_partition *partition) {
    int fd = 0;
    ledger_status rc;
    char *lock_path = NULL;
    ssize_t path_len;

    path_len = ledger_concat_path(partition->path, LOCK_FILE, &lock_path);
    ledger_check_rc(path_len > 0, LEDGER_ERR_MEMORY, "Failed to build lock path");

    fd = open(lock_path, O_RDWR|O_CREAT, 0700);
    ledger_check_rc(fd > 0 || errno == EEXIST, LEDGER_ERR_BAD_LOCKFILE, "Failed to open meta file");

    free(lock_path);
    return fd;

error:
    if(lock_path) {
        free(lock_path);
    }
    if(fd) {
        close(fd);
    }
    return rc;
}

static ledger_status create_locks(ledger_partition *partition, int fd) {
    ledger_status rc;
    ledger_partition_locks locks;
    pthread_mutexattr_t mattr;
    pthread_condattr_t cattr;

    rc = pthread_mutexattr_init(&mattr);
    ledger_check_rc(rc == 0, LEDGER_ERR_GENERAL, "Failed to initialize mutex attribute");

    rc = pthread_condattr_init(&cattr);
    ledger_check_rc(rc == 0, LEDGER_ERR_GENERAL, "Failed to initialize cond attribute");

    rc = pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    ledger_check_rc(rc == 0, LEDGER_ERR_GENERAL, "Failed to set mutex attribute to shared");

    rc = pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
    ledger_check_rc(rc == 0, LEDGER_ERR_GENERAL, "Failed to set cond attribute to shared");

    rc = pthread_mutex_init(&locks.rotate_lock, &mattr);
    ledger_check_rc(rc == 0, LEDGER_ERR_GENERAL, "Failed to initialize journal rotate mutex");

    rc = pthread_cond_init(&locks.rotate_cond, &cattr);
    ledger_check_rc(rc == 0, LEDGER_ERR_GENERAL, "Failed to initialize journal rotate cond");

    rc = ledger_pwrite(fd, (void *)&locks, sizeof(ledger_partition_locks), 0);
    ledger_check_rc(rc, LEDGER_ERR_IO, "Failed to write meta number of entries");

    return LEDGER_OK;

error:
    return rc;
}

static ledger_status map_lockfile(ledger_partition *partition, int fd, size_t map_len) {
    ledger_status rc;
    void *map = NULL;

    map = mmap(NULL, map_len, PROT_READ|PROT_WRITE,
               MAP_SHARED, fd, 0);
    ledger_check_rc(map != MAP_FAILED, LEDGER_ERR_IO, "Failed to memory map the lock file");

    partition->lockfile.locks = (ledger_partition_locks *)map;
    partition->lockfile.map = map;
    partition->lockfile.map_len = map_len;

    return LEDGER_OK;

error:
    if(map) {
        munmap(map, map_len);
    }
    return rc;
}

static ledger_status unmap_lockfile(ledger_partition *partition) {
    if(partition->lockfile.map) {
        munmap(partition->lockfile.map, partition->lockfile.map_len);
    }
    return LEDGER_OK;
}

static ledger_journal_meta_entry *find_meta(ledger_partition *partition, uint64_t message_id) {
    ledger_journal_meta_entry *entry = NULL;
    int i;

    for(i = partition->meta.nentries-1; i >= 0; i--) {
        entry = &partition->meta.entries[i];
        if(entry->first_message_id <= message_id) {
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
    ledger_status rc = LEDGER_OK;
    int fd = 0;
    int new_fd = 0;
    struct stat st;

    fd = open_meta(partition);
    ledger_check_rc(fd > 0, rc, "Failed to open meta file");

    rc = remove_old_journals(partition, fd, &new_fd);
    ledger_check_rc(rc == LEDGER_OK, rc, "Failed to remove old journals");

    rc = add_journal(partition, new_fd);
    ledger_check_rc(rc == LEDGER_OK, rc, "Failed to add a journal");

    rc = fstat(new_fd, &st);
    ledger_check_rc(rc == 0, LEDGER_ERR_IO, "Failed to stat meta file");

    rc = remap_meta(partition, new_fd, st.st_size);
    ledger_check_rc(rc == LEDGER_OK, rc, "Failed to remap meta during rotation");

    close(new_fd);
    return LEDGER_OK;

error:
    if(fd) {
        close(fd);
    }
    if(new_fd) {
        close(new_fd);
    }
    return rc;
}

ledger_status ledger_partition_latest_message_id(ledger_partition *partition, uint64_t *id) {
    ledger_journal_meta_entry *latest_meta;
    ledger_journal journal;
    ledger_journal_options journal_options;
    ledger_status rc;

    latest_meta = find_latest_meta(partition);
    if(latest_meta == NULL) {
        *id = 0;
        return LEDGER_OK;
    }

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

ledger_status ledger_partition_open(ledger_partition *partition, const char *topic_path,
                                    unsigned int partition_number, ledger_partition_options *options) {
    ledger_status rc;
    int fd = 0;
    int lock_fd = 0;
    char part_num[5];
    ssize_t path_len;
    char *partition_path = NULL;
    struct stat st;

    partition->path = NULL;
    partition->opened = false;
    partition->number = partition_number;

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
        rc = add_journal(partition, fd);
        ledger_check_rc(rc == LEDGER_OK, rc, "Failed to add initial journal");

        rc = fstat(fd, &st);
        ledger_check_rc(rc == 0, LEDGER_ERR_IO, "Failed to restat meta file");
    }

    rc = remap_meta(partition, fd, st.st_size);
    ledger_check_rc(rc == LEDGER_OK, rc, "Failed to read memory mapped meta file");

    lock_fd = open_lockfile(partition);
    ledger_check_rc(lock_fd > 0, rc, "Failed to open lockfile");

    rc = fstat(lock_fd, &st);
    ledger_check_rc(rc == 0, LEDGER_ERR_IO, "Failed to stat lockfile");

    if(st.st_size == 0) {
        rc = create_locks(partition, lock_fd);
        ledger_check_rc(rc == LEDGER_OK, rc, "Failed to create partition locks");

        rc = fstat(lock_fd, &st);
        ledger_check_rc(rc == 0, LEDGER_ERR_IO, "Failed to restat lock file");
    }

    rc = map_lockfile(partition, lock_fd, st.st_size);
    ledger_check_rc(rc == LEDGER_OK, rc, "Failed to read memory mapped lock file");

    close(fd);
    close(lock_fd);

    ledger_signal_init(&partition->message_signal);

    partition->opened = true;

    return LEDGER_OK;

error:
    if(partition_path) {
        free(partition_path);
    }
    if(fd) {
        close(fd);
    }
    if(lock_fd) {
        close(lock_fd);
    }
    return rc;
}

ledger_status ledger_partition_write(ledger_partition *partition, void *data,
                                     size_t len, ledger_write_status *status) {
    ledger_status rc, write_status;
    ledger_journal_meta_entry *latest_meta = NULL;
    ledger_journal journal;
    ledger_journal_options journal_options;

    ledger_check_rc(partition->meta.nentries > 0, LEDGER_ERR_BAD_PARTITION, "No journal entry to write to");

    journal_options.drop_corrupt = partition->options.drop_corrupt;
    journal_options.max_size_bytes = partition->options.journal_max_size_bytes;

    if(status != NULL) {
        status->partition_num = partition->number;
    }

    do {
        latest_meta = find_latest_meta(partition);

        rc = pthread_mutex_lock(&latest_meta->write_lock);
        ledger_check_rc(rc == 0, LEDGER_ERR_GENERAL, "Failed to lock partition for writing");

        rc = ledger_journal_open(&journal, partition->path, latest_meta, &journal_options);
        ledger_check_rc(rc == LEDGER_OK, rc, "Failed to open journal");

        rc = ledger_journal_write(&journal, data, len, status);
        ledger_check_rc(rc == LEDGER_OK || rc == LEDGER_NEXT, rc, "Failed to write to journal");

        write_status = rc;
        if(write_status == LEDGER_NEXT) {
            rc = rotate_journals(partition);
            ledger_check_rc(rc == LEDGER_OK, rc, "Failed to rotate journals");
        }

        // Signal any waiting consumers that there are messages available
        ledger_partition_signal_readers(partition);
        rc = pthread_mutex_unlock(&latest_meta->write_lock);
        ledger_check_rc(rc == 0, LEDGER_ERR_GENERAL, "Failed to unlock partition for writing");

        ledger_journal_close(&journal);
    } while (write_status == LEDGER_NEXT);
    return LEDGER_OK;

error:
    pthread_mutex_unlock(&latest_meta->write_lock);
    ledger_journal_close(&journal);
    return rc;
}

void ledger_partition_wait_messages(ledger_partition *partition) {
    ledger_signal_wait(&partition->message_signal);
}

void ledger_partition_signal_readers(ledger_partition *partition) {
    ledger_signal_broadcast(&partition->message_signal);
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

    messages->partition_num = partition->number;
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
    unmap_lockfile(partition);
    partition->opened = false;
}

