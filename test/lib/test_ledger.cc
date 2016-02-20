#include <gtest/gtest.h>

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ftw.h>
#include <fcntl.h>

#include "ledger.h"

namespace ledger_test {
static const char *WORKING_DIR = "/tmp/ledger";
static const char *CORRUPT_WORKING_DIR = "/tmp/corrupt_ledger";
static const char *TOPIC = "my_data";
static const char *FULL_TOPIC = "/tmp/ledger/my_data";
static const char *FULL_PARTITION = "/tmp/ledger/my_data/0";

static int setup(const char *directory) {
    return mkdir(directory, 0777);
}

static int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    int rv = remove(fpath);

    if (rv) perror(fpath);

    return rv;
}

int rmrf(const char *path) {
    return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}

static int cleanup(const char *directory) {
    return rmrf(directory);
}

TEST(Ledger, CreatesCorrectDirectories) {
    ledger_ctx ctx;
    ledger_topic_options options;
    DIR *dir;
    struct dirent *dit;

    cleanup(WORKING_DIR);
    ASSERT_EQ(0, setup(WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_context(&ctx, WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_topic_options_init(&options));
    unsigned int partition_ids[] = {0, 1};
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, partition_ids, 2, &options));

    dir = opendir(WORKING_DIR);
    ASSERT_TRUE(dir != NULL);

    bool seen_topic_dir = false;
    while((dit = readdir(dir)) != NULL) {
        if(strcmp(TOPIC, dit->d_name) == 0) {
            seen_topic_dir = true;
        }
    }
    EXPECT_TRUE(seen_topic_dir);
    ASSERT_EQ(0, closedir(dir));

    dir = opendir(FULL_TOPIC);
    ASSERT_TRUE(dir != NULL);

    int partition_count = 0;
    while((dit = readdir(dir)) != NULL) {
        partition_count++;
    }
    EXPECT_EQ(4, partition_count);

    ledger_close_context(&ctx);
    ASSERT_EQ(0, closedir(dir));
    ASSERT_EQ(0, cleanup(WORKING_DIR));
}

TEST(Ledger, BadWrites) {
    ledger_ctx ctx;
    ledger_topic_options options;
    const char message[] = "hello";
    size_t mlen = sizeof(message);

    cleanup(WORKING_DIR);
    ASSERT_EQ(0, setup(WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_context(&ctx, WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_topic_options_init(&options));
    EXPECT_EQ(LEDGER_ERR_BAD_TOPIC, ledger_write_partition(&ctx, "bad-topic", 0, (void *)message, mlen, NULL));

    unsigned int partition_ids[] = {0, 1};
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, partition_ids, 2, &options));
    EXPECT_EQ(LEDGER_ERR_BAD_PARTITION, ledger_write_partition(&ctx, TOPIC, 5, (void *)message, mlen, NULL));

    ledger_close_context(&ctx);
    ASSERT_EQ(0, cleanup(WORKING_DIR));
}

TEST(Ledger, WriteWithHashing) {
   ledger_ctx ctx;
    ledger_topic_options options;
    const char message[] = "hello";
    size_t mlen = sizeof(message);
    ledger_message_set messages;
    ledger_write_status status;

    cleanup(WORKING_DIR);
    ASSERT_EQ(0, setup(WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_context(&ctx, WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_topic_options_init(&options));
    unsigned int partition_ids[] = {0, 1, 2, 3, 4};
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, partition_ids, 5, &options));

    ASSERT_EQ(LEDGER_OK, ledger_write(&ctx, TOPIC, "hello_msg", 9, (void *)message, mlen, &status));
    ASSERT_EQ(2, status.partition_num);

    EXPECT_EQ(LEDGER_OK, ledger_read_partition(&ctx, TOPIC, status.partition_num, LEDGER_BEGIN, LEDGER_CHUNK_SIZE, &messages));
    EXPECT_EQ(1, messages.nmessages);
    EXPECT_EQ(mlen, messages.messages[0].len);
    EXPECT_EQ(0, messages.messages[0].id);
    EXPECT_STREQ(message, (const char *)messages.messages[0].data);

    ledger_message_set_free(&messages);
    ledger_close_context(&ctx);
    ASSERT_EQ(0, cleanup(WORKING_DIR));
}

TEST(Ledger, CorrectWritesSingleTopic) {
    ledger_ctx ctx;
    ledger_topic_options options;
    const char message[] = "hello";
    size_t mlen = sizeof(message);
    ledger_message_set messages;
    ledger_write_status status;

    cleanup(WORKING_DIR);
    ASSERT_EQ(0, setup(WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_context(&ctx, WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_topic_options_init(&options));
    unsigned int partition_ids[] = {0};
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, partition_ids, 1, &options));

    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message, mlen, &status));
    EXPECT_EQ(0, status.message_id);
    EXPECT_EQ(LEDGER_OK, ledger_read_partition(&ctx, TOPIC, 0, LEDGER_BEGIN, LEDGER_CHUNK_SIZE, &messages));
    EXPECT_EQ(1, messages.nmessages);
    EXPECT_EQ(mlen, messages.messages[0].len);
    EXPECT_EQ(0, messages.messages[0].id);
    EXPECT_STREQ(message, (const char *)messages.messages[0].data);
    EXPECT_EQ(1, messages.next_id);

    ledger_message_set_free(&messages);
    ledger_close_context(&ctx);
    ASSERT_EQ(0, cleanup(WORKING_DIR));
}

TEST(Ledger, LatestMessageId) {
    ledger_ctx ctx;
    ledger_topic_options options;
    const char message[] = "hello";
    size_t mlen = sizeof(message);
    ledger_message_set messages;
    ledger_write_status status;
    uint64_t latest_id;

    cleanup(WORKING_DIR);
    ASSERT_EQ(0, setup(WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_context(&ctx, WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_topic_options_init(&options));
    unsigned int partition_ids[] = {0};
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, partition_ids, 1, &options));

    ASSERT_EQ(LEDGER_OK, ledger_latest_message_id(&ctx, TOPIC, 0, &latest_id));
    EXPECT_EQ(0, latest_id);

    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message, mlen, &status));
    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message, mlen, &status));
    EXPECT_EQ(1, status.message_id);
    ASSERT_EQ(LEDGER_OK, ledger_latest_message_id(&ctx, TOPIC, 0, &latest_id));
    EXPECT_EQ(2, latest_id);

    ledger_close_context(&ctx);
    ASSERT_EQ(0, cleanup(WORKING_DIR));
}

TEST(Ledger, BadTopicOpen) {
    ledger_ctx ctx;
    ledger_topic_options options;

    cleanup(WORKING_DIR);
    ASSERT_EQ(0, setup(WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_context(&ctx, WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_topic_options_init(&options));
    unsigned int pid1[] = {};
    unsigned int partition_ids[] = {0};
    ASSERT_EQ(LEDGER_ERR_BAD_TOPIC, ledger_open_topic(&ctx, TOPIC, pid1, 0, &options));
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, partition_ids, 1, &options));
    ASSERT_EQ(LEDGER_ERR_BAD_TOPIC, ledger_open_topic(&ctx, TOPIC, partition_ids, 1, &options));

    ledger_close_context(&ctx);
    ASSERT_EQ(0, cleanup(WORKING_DIR));
}

TEST(Ledger, LookupTopic) {
    ledger_ctx ctx;
    ledger_topic *topic;
    ledger_topic_options options;

    cleanup(WORKING_DIR);
    ASSERT_EQ(0, setup(WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_context(&ctx, WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_topic_options_init(&options));
    unsigned int partition_ids[] = {0};
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, partition_ids, 1, &options));

    topic = ledger_lookup_topic(&ctx, TOPIC);
    EXPECT_TRUE(topic != NULL);
    EXPECT_STREQ(TOPIC, topic->name);
    EXPECT_EQ(1, topic->npartitions);

    topic = ledger_lookup_topic(&ctx, "NOT_FOUND");
    EXPECT_EQ(NULL, topic);

    ledger_close_context(&ctx);
    ASSERT_EQ(0, cleanup(WORKING_DIR));
}

TEST(Ledger, HeapAllocatedTopicNames) {
    ledger_ctx ctx;
    ledger_topic_options options;
    const char message[] = "hello";
    size_t mlen = sizeof(message);
    ledger_message_set messages;
    ledger_write_status status;

    // Make the open topic call memory be different than the write call
    size_t tsize = strlen(TOPIC) * sizeof(char);
    char *topic_open = (char *)malloc(tsize);
    ASSERT_TRUE(topic_open != NULL);
    strncpy(topic_open, TOPIC, tsize);

    cleanup(WORKING_DIR);
    ASSERT_EQ(0, setup(WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_context(&ctx, WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_topic_options_init(&options));
    unsigned int partition_ids[] = {0};
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, topic_open, partition_ids, 1, &options));
    // Now destroy the heap allocation to enforce the correct topic name ownershipe
    free(topic_open);

    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message, mlen, &status));
    EXPECT_EQ(0, status.message_id);
    EXPECT_EQ(LEDGER_OK, ledger_read_partition(&ctx, TOPIC, 0, LEDGER_BEGIN, LEDGER_CHUNK_SIZE, &messages));
    EXPECT_EQ(1, messages.nmessages);
    EXPECT_EQ(mlen, messages.messages[0].len);
    EXPECT_EQ(0, messages.messages[0].id);
    EXPECT_STREQ(message, (const char *)messages.messages[0].data);
    EXPECT_EQ(1, messages.next_id);

    ledger_message_set_free(&messages);
    ledger_close_context(&ctx);
    ASSERT_EQ(0, cleanup(WORKING_DIR));
}

TEST(Ledger, ReadEndOfJournal) {
    ledger_ctx ctx;
    ledger_topic_options options;
    const char message[] = "hello";
    size_t mlen = sizeof(message);
    ledger_message_set messages;
    ledger_write_status status;

    cleanup(WORKING_DIR);
    ASSERT_EQ(0, setup(WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_context(&ctx, WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_topic_options_init(&options));
    unsigned int partition_ids[] = {0};
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, partition_ids, 1, &options));

    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message, mlen, NULL));
    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message, mlen, NULL));
    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message, mlen, NULL));
    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message, mlen, &status));
    EXPECT_EQ(3, status.message_id);

    EXPECT_EQ(LEDGER_OK, ledger_read_partition(&ctx, TOPIC, 0, LEDGER_END, LEDGER_CHUNK_SIZE, &messages));
    EXPECT_EQ(4, messages.next_id);
    EXPECT_EQ(0, messages.nmessages);

    ledger_message_set_free(&messages);
    ledger_close_context(&ctx);
    ASSERT_EQ(0, cleanup(WORKING_DIR));
}

TEST(Ledger, CorruptMessage) {
    ledger_ctx ctx;
    ledger_topic_options options;
    const char message1[] = "hello";
    const char message2[] = "there";
    const char message3[] = "friend";
    ledger_message_set messages;
    ledger_write_status status;
    int fd;

    cleanup(CORRUPT_WORKING_DIR);
    ASSERT_EQ(0, setup(CORRUPT_WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_context(&ctx, CORRUPT_WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_topic_options_init(&options));
    options.drop_corrupt = true;
    unsigned int partition_ids[] = {0};
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, partition_ids, 1, &options));

    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message1, sizeof(message1), NULL));
    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message2, sizeof(message2), NULL));
    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message3, sizeof(message3), &status));
    EXPECT_EQ(2, status.message_id);

    // Corrupt the middle message out of band.
    fd = open("/tmp/corrupt_ledger/my_data/0/00000000.jnl", O_RDWR, 0755);
    ASSERT_TRUE(fd > 0);
    ASSERT_EQ(1, pwrite(fd, "i", 1, 26));
    close(fd);

    EXPECT_EQ(LEDGER_OK, ledger_read_partition(&ctx, TOPIC, 0, LEDGER_BEGIN, LEDGER_CHUNK_SIZE, &messages));
    EXPECT_EQ(2, messages.nmessages);
    EXPECT_EQ(3, messages.next_id);

    EXPECT_EQ(sizeof(message1), messages.messages[0].len);
    EXPECT_EQ(0, messages.messages[0].id);
    EXPECT_STREQ(message1, (const char *)messages.messages[0].data);

    EXPECT_EQ(sizeof(message3), messages.messages[1].len);
    EXPECT_EQ(2, messages.messages[1].id);
    EXPECT_STREQ(message3, (const char *)messages.messages[1].data);

    ledger_message_set_free(&messages);
    ledger_close_context(&ctx);
    ASSERT_EQ(0, cleanup(CORRUPT_WORKING_DIR));
}

TEST(Ledger, DoubleCorruptMessage) {
    ledger_ctx ctx;
    ledger_topic_options options;
    const char message1[] = "hello";
    const char message2[] = "there";
    const char message3[] = "friend";
    ledger_message_set messages;
    int fd;

    cleanup(CORRUPT_WORKING_DIR);
    ASSERT_EQ(0, setup(CORRUPT_WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_context(&ctx, CORRUPT_WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_topic_options_init(&options));
    options.drop_corrupt = true;
    unsigned int partition_ids[] = {0};
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, partition_ids, 1, &options));

    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message1, sizeof(message1), NULL));
    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message2, sizeof(message2), NULL));
    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message3, sizeof(message3), NULL));

    // Corrupt the last two messages out of band
    fd = open("/tmp/corrupt_ledger/my_data/0/00000000.jnl", O_RDWR, 0755);
    ASSERT_TRUE(fd > 0);
    ASSERT_EQ(1, pwrite(fd, "i", 1, 26));
    ASSERT_EQ(1, pwrite(fd, "b", 1, 36));
    close(fd);

    EXPECT_EQ(LEDGER_OK, ledger_read_partition(&ctx, TOPIC, 0, LEDGER_BEGIN, LEDGER_CHUNK_SIZE, &messages));
    EXPECT_EQ(1, messages.nmessages);
    EXPECT_EQ(3, messages.next_id);

    EXPECT_EQ(sizeof(message1), messages.messages[0].len);
    EXPECT_EQ(0, messages.messages[0].id);
    EXPECT_STREQ(message1, (const char *)messages.messages[0].data);

    ledger_message_set_free(&messages);
    ledger_close_context(&ctx);
    ASSERT_EQ(0, cleanup(CORRUPT_WORKING_DIR));
}

TEST(Ledger, MultipleMessagesAtOffset) {
    ledger_ctx ctx;
    ledger_topic_options options;
    const char message1[] = "hello";
    const char message2[] = "there";
    const char message3[] = "my";
    const char message4[] = "friend";
    ledger_message_set messages;

    cleanup(WORKING_DIR);
    ASSERT_EQ(0, setup(WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_context(&ctx, WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_topic_options_init(&options));
    unsigned int partition_ids[] = {0};
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, partition_ids, 1, &options));

    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message1, sizeof(message1), NULL));
    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message2, sizeof(message2), NULL));
    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message3, sizeof(message3), NULL));
    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message4, sizeof(message4), NULL));

    EXPECT_EQ(LEDGER_OK, ledger_read_partition(&ctx, TOPIC, 0, 1, LEDGER_CHUNK_SIZE, &messages));
    EXPECT_EQ(3, messages.nmessages);
    EXPECT_EQ(4, messages.next_id);

    EXPECT_EQ(sizeof(message2), messages.messages[0].len);
    EXPECT_EQ(1, messages.messages[0].id);
    EXPECT_STREQ(message2, (const char *)messages.messages[0].data);

    EXPECT_EQ(sizeof(message3), messages.messages[1].len);
    EXPECT_EQ(2, messages.messages[1].id);
    EXPECT_STREQ(message3, (const char *)messages.messages[1].data);

    EXPECT_EQ(sizeof(message4), messages.messages[2].len);
    EXPECT_EQ(3, messages.messages[2].id);
    EXPECT_STREQ(message4, (const char *)messages.messages[2].data);

    ledger_message_set_free(&messages);
    ledger_close_context(&ctx);
    ASSERT_EQ(0, cleanup(WORKING_DIR));
}

TEST(Ledger, ReadMultiplePartitions) {
    ledger_ctx ctx;
    ledger_topic_options options;
    const char message1[] = "hello";
    const char message2[] = "there";
    ledger_message_set messages;

    cleanup(WORKING_DIR);
    ASSERT_EQ(0, setup(WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_context(&ctx, WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_topic_options_init(&options));
    unsigned int partition_ids[] = {0, 1};
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, partition_ids, 2, &options));

    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message1, sizeof(message1), NULL));
    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 1, (void *)message2, sizeof(message2), NULL));

    EXPECT_EQ(LEDGER_OK, ledger_read_partition(&ctx, TOPIC, 0, 0, 1, &messages));
    EXPECT_EQ(1, messages.nmessages);
    EXPECT_EQ(1, messages.next_id);
    EXPECT_EQ(0, messages.partition_num);

    ledger_message_set_free(&messages);

    EXPECT_EQ(LEDGER_OK, ledger_read_partition(&ctx, TOPIC, 1, 0, 1, &messages));
    EXPECT_EQ(1, messages.nmessages);
    EXPECT_EQ(1, messages.next_id);
    EXPECT_EQ(1, messages.partition_num);

    ledger_message_set_free(&messages);

    ledger_close_context(&ctx);
    ASSERT_EQ(0, cleanup(WORKING_DIR));
}

TEST(Ledger, SmallerRead) {
    ledger_ctx ctx;
    ledger_topic_options options;
    const char message1[] = "hello";
    const char message2[] = "there";
    ledger_message_set messages;

    cleanup(WORKING_DIR);
    ASSERT_EQ(0, setup(WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_context(&ctx, WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_topic_options_init(&options));
    unsigned int partition_ids[] = {0};
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, partition_ids, 1, &options));

    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message1, sizeof(message1), NULL));
    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message2, sizeof(message2), NULL));

    EXPECT_EQ(LEDGER_OK, ledger_read_partition(&ctx, TOPIC, 0, 1, 1, &messages));
    EXPECT_EQ(1, messages.nmessages);
    EXPECT_EQ(2, messages.next_id);

    EXPECT_EQ(sizeof(message2), messages.messages[0].len);
    EXPECT_EQ(1, messages.messages[0].id);
    EXPECT_STREQ(message2, (const char *)messages.messages[0].data);

    ledger_message_set_free(&messages);
    ledger_close_context(&ctx);
    ASSERT_EQ(0, cleanup(WORKING_DIR));
}

TEST(Ledger, JournalRotation) {
    ledger_ctx ctx;
    ledger_topic_options options;
    const char message1[] = "hello";
    size_t mlen = sizeof(message1);
    ledger_message_set messages;
    int messages_count;
    int i;
    DIR *dir;
    struct dirent *dit;

    cleanup(WORKING_DIR);
    ASSERT_EQ(0, setup(WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_context(&ctx, WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_topic_options_init(&options));
    options.journal_max_size_bytes = 100;
    unsigned int partition_ids[] = {0};
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, partition_ids, 1, &options));

    // Write enough messages to force a journal rotation
    messages_count = 10;
    for(i = 0; i < messages_count; i++) {
        ASSERT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message1, mlen, NULL));
    }

    EXPECT_EQ(LEDGER_OK, ledger_read_partition(&ctx, TOPIC, 0, LEDGER_BEGIN, messages_count, &messages));
    EXPECT_EQ(messages_count, messages.nmessages);

    // Make sure they all have the correct content, and correct ids
    uint64_t last_id = -1;
    for(i = 0; i < messages_count; i++) {
        ASSERT_TRUE(messages.messages[i].id-last_id == 1);
        ASSERT_STREQ(message1, (const char *)messages.messages[i].data);
        ASSERT_EQ(mlen, messages.messages[i].len);
        last_id = messages.messages[i].id;
    }

    dir = opendir(FULL_PARTITION);
    ASSERT_TRUE(dir != NULL);

    int journal_count = 0;
    while((dit = readdir(dir)) != NULL) {
        journal_count++;
    }
    EXPECT_EQ(8, journal_count);

    ledger_message_set_free(&messages);
    ASSERT_EQ(0, closedir(dir));
    ledger_close_context(&ctx);
    ASSERT_EQ(0, cleanup(WORKING_DIR));
}

TEST(Ledger, DISABLED_JournalPurges) {
    ledger_ctx ctx;
    ledger_topic_options options;
    const char message1[] = "hello";
    size_t mlen = sizeof(message1);
    ledger_message_set messages;
    int messages_count;
    int i;
    DIR *dir;
    struct dirent *dit;

    cleanup(WORKING_DIR);
    ASSERT_EQ(0, setup(WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_context(&ctx, WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_topic_options_init(&options));
    options.journal_max_size_bytes = 100;
    options.journal_purge_age_seconds = 1;
    unsigned int partition_ids[] = {0};
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, partition_ids, 1, &options));

    // Write enough messages to force journal rotations
    messages_count = 10;
    for(i = 0; i < messages_count; i++) {
        ASSERT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message1, mlen, NULL));
    }

    sleep(2);

    // Write enough again. Old journals should be purged.
    for(i = 0; i < messages_count; i++) {
        ASSERT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message1, mlen, NULL));
    }

    EXPECT_EQ(LEDGER_OK, ledger_read_partition(&ctx, TOPIC, 0, LEDGER_BEGIN, messages_count*2, &messages));
    EXPECT_EQ(messages_count, messages.nmessages);

    dir = opendir(FULL_PARTITION);
    ASSERT_TRUE(dir != NULL);

    int journal_count = 0;
    while((dit = readdir(dir)) != NULL) {
        journal_count++;
    }
    EXPECT_EQ(8, journal_count);

    ledger_message_set_free(&messages);
    ASSERT_EQ(0, closedir(dir));
    ledger_close_context(&ctx);
    ASSERT_EQ(0, cleanup(WORKING_DIR));
}

}
