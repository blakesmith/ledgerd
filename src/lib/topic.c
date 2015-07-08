#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "common.h"
#include "topic.h"

ledger_status ledger_topic_new(const char *name, ledger_topic **topic_out) {
    size_t tlen;
    ledger_status rc;
    ledger_topic *topic;
    char *tname = NULL;
    
    topic = malloc(sizeof(ledger_topic));
    ledger_check_rc(topic != NULL, LEDGER_ERR_MEMORY, "Failed to allocate topic");
    tlen = strlen(name)+1;
    tname = malloc(tlen);
    ledger_check_rc(tname != NULL, LEDGER_ERR_MEMORY, "Failed to allocate topic name");
    strncpy(tname, name, tlen);
    topic->name = tname;
    *topic_out = topic;
    
    return LEDGER_OK;

error:
    if(topic) {
        free(topic);
    }
    if(tname) {
        free(tname);
    }
    return rc;
}

ledger_status ledger_topic_options_init(ledger_topic_options *options) {
    options->drop_corrupt = false;
    options->journal_max_size_bytes = DEFAULT_JOURNAL_MAX_SIZE;

    return LEDGER_OK;
}

ledger_status ledger_topic_open(ledger_topic *topic, const char *root,
                                unsigned int partition_count,
                                ledger_topic_options *options) {
    int i;
    ledger_status rc;
    ssize_t path_len;
    ledger_partition *partition = NULL;
    ledger_partition_options partition_options;
    char *topic_path = NULL;

    topic->opened = false;
    topic->path = NULL;

    ledger_check_rc(partition_count < MAX_PARTITIONS, LEDGER_ERR_ARGS, "Too many partitions");

    path_len = ledger_concat_path(root, topic->name, &topic_path);
    ledger_check_rc(path_len > 0, path_len, "Failed to construct directory path");

    memcpy(&topic->options, options, sizeof(ledger_topic_options));
    topic->path_len = path_len;
    topic->path = topic_path;
    topic->npartitions = partition_count;

    rc = mkdir(topic_path, 0755);
    ledger_check_rc(rc == 0 || errno == EEXIST, LEDGER_ERR_MKDIR, "Failed to create context directory");

    topic->partitions = ledger_reallocarray(NULL, partition_count, sizeof(ledger_partition));
    ledger_check_rc(topic->partitions != NULL, LEDGER_ERR_MEMORY, "Failed to allocate partitions");

    for(i = 0; i < partition_count; i++) {
        partition = &topic->partitions[i];
        partition_options.drop_corrupt = options->drop_corrupt;
        partition_options.journal_max_size_bytes = options->journal_max_size_bytes;

        rc = ledger_partition_open(partition, topic_path, i, &partition_options);
        ledger_check_rc(rc == LEDGER_OK, rc, "Failed to open partition");
    }

    topic->opened = true;

    return LEDGER_OK;

error:
    if(topic_path) {
        free(topic_path);
    }
    return rc;
}

ledger_status ledger_topic_write_partition(ledger_topic *topic, unsigned int partition_num,
                                           void *data, size_t len, ledger_write_status *status) {
    ledger_status rc;
    ledger_partition *partition;

    ledger_check_rc(partition_num < topic->npartitions, LEDGER_ERR_BAD_PARTITION, "Write to unknown partition");
    partition = &topic->partitions[partition_num];

    return ledger_partition_write(partition, data, len, status);

error:
    return rc;
}

ledger_status ledger_topic_read_partition(ledger_topic *topic, unsigned int partition_num,
                                          uint64_t start_id, size_t nmessages,
                                          ledger_message_set *messages) {
    ledger_status rc;
    ledger_partition *partition;

    ledger_check_rc(partition_num < topic->npartitions, LEDGER_ERR_BAD_PARTITION, "Write to unknown partition");
    partition = &topic->partitions[partition_num];

    return ledger_partition_read(partition, start_id, nmessages, messages);

error:
    return rc;
}

ledger_status ledger_topic_wait_messages(ledger_topic *topic, unsigned int partition_num) {
    ledger_status rc;
    ledger_partition *partition;

    ledger_check_rc(partition_num < topic->npartitions, LEDGER_ERR_BAD_PARTITION, "Waiting on unknown partition");
    partition = &topic->partitions[partition_num];

    ledger_partition_wait_messages(partition);

    return LEDGER_OK;

error:
    return rc;
}

ledger_status ledger_topic_signal_readers(ledger_topic *topic, unsigned int partition_num) {
    ledger_status rc;
    ledger_partition *partition;

    ledger_check_rc(partition_num < topic->npartitions, LEDGER_ERR_BAD_PARTITION, "Waiting on unknown partition");
    partition = &topic->partitions[partition_num];

    ledger_partition_signal_readers(partition);

    return LEDGER_OK;

error:
    return rc;
}

void ledger_topic_close(ledger_topic *topic) {
    int i;
    ledger_partition *partition;

    if(topic->opened) {
        for(i = 0; i < topic->npartitions; i++) {
            partition = topic->partitions + i;
            ledger_partition_close(partition);
        }
        free(topic->partitions);
        if(topic->path) {
            free(topic->path);
        }
    }
    free(topic->name);
    topic->opened = false;
}
