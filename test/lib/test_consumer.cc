#include <gtest/gtest.h>

#include <ftw.h>

#include "ledger.h"

namespace ledger_consumer_test {
static const char *WORKING_DIR = "/tmp/ledger";
static const char *TOPIC = "my_data";

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

static ledger_consume_status consume_function(ledger_consumer_ctx *ctx, ledger_message_set *messages, void *data) {
    bool *consumed = static_cast<bool*>(data);
    *consumed = true;
    return LEDGER_CONSUMER_OK;
}

TEST(LedgerConsumer, ConsumingSinglePartitionNoThreading) {
    ledger_ctx ctx;
    ledger_topic_options options;
    ledger_consumer consumer;
    ledger_consumer_options consumer_opts;

    cleanup(WORKING_DIR);
    ASSERT_EQ(0, setup(WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_context(&ctx, WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_topic_options_init(&options));
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, 1, &options));

    bool consumed = false;
    ASSERT_EQ(LEDGER_OK, ledger_init_consumer_options(&consumer_opts));
    consumer_opts.read_chunk_size = 50;
    ASSERT_EQ(LEDGER_OK, ledger_consumer_init(&consumer, consume_function, &consumer_opts, &consumed));
    ASSERT_EQ(LEDGER_OK, ledger_attach_consumer(&ctx, &consumer, TOPIC, 0));
    
    EXPECT_EQ(LEDGER_OK, ledger_consumer_consume_chunk(&consumer));
    EXPECT_TRUE(consumed);

    ledger_consumer_close(&consumer);
    ledger_close_context(&ctx);
    ASSERT_EQ(0, cleanup(WORKING_DIR));
}

}
