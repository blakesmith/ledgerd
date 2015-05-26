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
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, 2, &options));

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
    EXPECT_EQ(LEDGER_ERR_BAD_TOPIC, ledger_write_partition(&ctx, "bad-topic", 0, (void *)message, mlen));

    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, 2, &options));
    EXPECT_EQ(LEDGER_ERR_BAD_PARTITION, ledger_write_partition(&ctx, TOPIC, 5, (void *)message, mlen));

    ledger_close_context(&ctx);
    ASSERT_EQ(0, cleanup(WORKING_DIR));
}

TEST(Ledger, CorrectWritesSingleTopic) {
    ledger_ctx ctx;
    ledger_topic_options options;
    const char message[] = "hello";
    size_t mlen = sizeof(message);
    ledger_message_set messages;

    cleanup(WORKING_DIR);
    ASSERT_EQ(0, setup(WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_context(&ctx, WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_topic_options_init(&options));
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, 1, &options));

    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message, mlen));
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

TEST(Ledger, CorruptMessage) {
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
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, 1, &options));

    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message1, sizeof(message1)));
    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message2, sizeof(message2)));
    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message3, sizeof(message3)));

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
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, 1, &options));

    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message1, sizeof(message1)));
    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message2, sizeof(message2)));
    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message3, sizeof(message3)));

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
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, 1, &options));

    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message1, sizeof(message1)));
    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message2, sizeof(message2)));
    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message3, sizeof(message3)));
    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message4, sizeof(message4)));

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
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, 1, &options));

    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message1, sizeof(message1)));
    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message2, sizeof(message2)));

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
}
