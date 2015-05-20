#include <gtest/gtest.h>

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ftw.h>

#include "ledger.h"

namespace ledger_test {
static const char *WORKING_DIR = "/tmp/ledger";
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
    DIR *dir;
    struct dirent *dit;

    cleanup(WORKING_DIR);
    ASSERT_EQ(0, setup(WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_context(&ctx, WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, 2, 0));

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
    const char message[] = "hello";
    size_t mlen = sizeof(message);

    cleanup(WORKING_DIR);
    ASSERT_EQ(0, setup(WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_context(&ctx, WORKING_DIR));
    EXPECT_EQ(LEDGER_ERR_BAD_TOPIC, ledger_write_partition(&ctx, "bad-topic", 0, (void *)message, mlen));

    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, 2, 0));
    EXPECT_EQ(LEDGER_ERR_BAD_PARTITION, ledger_write_partition(&ctx, TOPIC, 5, (void *)message, mlen));

    ledger_close_context(&ctx);
    ASSERT_EQ(0, cleanup(WORKING_DIR));
}

TEST(Ledger, CorrectWritesSingleTopic) {
    ledger_ctx ctx;
    const char message[] = "hello";
    size_t mlen = sizeof(message);
    ledger_message_set messages;

    cleanup(WORKING_DIR);
    ASSERT_EQ(0, setup(WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_context(&ctx, WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, 1, 0));

    EXPECT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)message, mlen));
    EXPECT_EQ(LEDGER_OK, ledger_read_partition(&ctx, TOPIC, 0, LEDGER_BEGIN, LEDGER_CHUNK_SIZE, &messages));
    EXPECT_EQ(1, messages.nmessages);

    ledger_message_set_free(&messages);
    ledger_close_context(&ctx);
    ASSERT_EQ(0, cleanup(WORKING_DIR));
}
}
