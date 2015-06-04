#include <gtest/gtest.h>

#include <ftw.h>

#include "consumer.h"

namespace ledger_consumer_test {
static const char *WORKING_DIR = "/tmp/consumer_ledger";
static const char *TOPIC = "my_consumer_data";

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
    size_t *consumed_size = static_cast<size_t*>(data);
    int i;

    for(i = 0; i < messages->nmessages; i++) {
        *consumed_size = *consumed_size + messages->messages[i].len;
    }

    return LEDGER_CONSUMER_OK;
}

static ledger_consume_status concat_consume_function(ledger_consumer_ctx *ctx, ledger_message_set *messages, void *data) {
    std::string *concated_str = static_cast<std::string*>(data);
    int i;

    for(i = 0; i < messages->nmessages; i++) {
        std::string new_message((const char *)messages->messages[i].data,
                                messages->messages[i].len);
        *concated_str = *concated_str + new_message;
    }

    return LEDGER_CONSUMER_OK;
}

TEST(LedgerConsumer, ConsumingSinglePartitionNoThreading) {
    ledger_ctx ctx;
    ledger_topic_options options;
    ledger_consumer consumer;
    ledger_consumer_options consumer_opts;
    ledger_write_status status;

    cleanup(WORKING_DIR);
    ASSERT_EQ(0, setup(WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_context(&ctx, WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_topic_options_init(&options));
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, 1, &options));

    ASSERT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)"hello", 5, NULL));
    ASSERT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)"there", 5, &status));

    size_t consumed_size = 0;
    ASSERT_EQ(LEDGER_OK, ledger_init_consumer_options(&consumer_opts));
    consumer_opts.read_chunk_size = 1;
    ASSERT_EQ(LEDGER_OK, ledger_consumer_init(&consumer, consume_function, &consumer_opts, &consumed_size));
    ASSERT_EQ(LEDGER_OK, ledger_consumer_attach(&consumer, &ctx, TOPIC, 0));
    EXPECT_EQ(LEDGER_OK, ledger_consumer_start(&consumer, 0));

    ledger_consumer_wait_for_position(&consumer, status.message_id);
    ledger_consumer_stop(&consumer);
    EXPECT_EQ(10, consumed_size);

    ledger_consumer_close(&consumer);
    ledger_close_context(&ctx);
    ASSERT_EQ(0, cleanup(WORKING_DIR));
}

TEST(LedgerConsumer, ConsumingMessagesWithDelay) {
    ledger_ctx ctx;
    ledger_topic_options options;
    ledger_consumer consumer;
    ledger_consumer_options consumer_opts;
    ledger_write_status status;

    cleanup(WORKING_DIR);
    ASSERT_EQ(0, setup(WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_context(&ctx, WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_topic_options_init(&options));
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, 1, &options));

    std::string concated_str;
    ASSERT_EQ(LEDGER_OK, ledger_init_consumer_options(&consumer_opts));
    consumer_opts.read_chunk_size = 2;
    ASSERT_EQ(LEDGER_OK, ledger_consumer_init(&consumer, concat_consume_function, &consumer_opts, &concated_str));
    ASSERT_EQ(LEDGER_OK, ledger_consumer_attach(&consumer, &ctx, TOPIC, 0));
    EXPECT_EQ(LEDGER_OK, ledger_consumer_start(&consumer, LEDGER_BEGIN));

    ASSERT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)"hello", 5, &status));
    ledger_consumer_wait_for_position(&consumer, status.message_id);

    ASSERT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)"there", 5, &status));

    ledger_consumer_wait_for_position(&consumer, status.message_id);
    ledger_consumer_stop(&consumer);
    EXPECT_EQ("hellothere", concated_str);

    ledger_consumer_close(&consumer);
    ledger_close_context(&ctx);
    ASSERT_EQ(0, cleanup(WORKING_DIR));
}

TEST(LedgerConsumer, ConsumingMessagesAtTheBeginningAndEnd) {
    ledger_ctx ctx;
    ledger_topic_options options;
    ledger_consumer consumer;
    ledger_consumer_options consumer_opts;
    ledger_write_status status;

    cleanup(WORKING_DIR);
    ASSERT_EQ(0, setup(WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_context(&ctx, WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_topic_options_init(&options));
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, 1, &options));

    std::string concated_str;
    ASSERT_EQ(LEDGER_OK, ledger_init_consumer_options(&consumer_opts));
    consumer_opts.read_chunk_size = 2;
    ASSERT_EQ(LEDGER_OK, ledger_consumer_init(&consumer, concat_consume_function, &consumer_opts, &concated_str));
    ASSERT_EQ(LEDGER_OK, ledger_consumer_attach(&consumer, &ctx, TOPIC, 0));
    EXPECT_EQ(LEDGER_OK, ledger_consumer_start(&consumer, LEDGER_END));
    // Not sure the best way to 'park' on the end.
    sleep(1);

    ASSERT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)"hello", 5, &status));
    ASSERT_EQ(0, status.message_id);
    ledger_consumer_wait_for_position(&consumer, status.message_id);

    ledger_consumer_stop(&consumer);
    EXPECT_EQ(0, ledger_consumer_position_get(&consumer.position));
    EXPECT_EQ("hello", concated_str);

    ledger_consumer_close(&consumer);
    ledger_close_context(&ctx);
    ASSERT_EQ(0, cleanup(WORKING_DIR));
}

TEST(LedgerConsumer, ConsumingMessagesAtTheEnd) {
    ledger_ctx ctx;
    ledger_topic_options options;
    ledger_consumer consumer;
    ledger_consumer_options consumer_opts;
    ledger_write_status status;

    cleanup(WORKING_DIR);
    ASSERT_EQ(0, setup(WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_context(&ctx, WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_topic_options_init(&options));
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, 1, &options));

    ASSERT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)"hello", 5, &status));

    std::string concated_str;
    ASSERT_EQ(LEDGER_OK, ledger_init_consumer_options(&consumer_opts));
    consumer_opts.read_chunk_size = 2;
    ASSERT_EQ(LEDGER_OK, ledger_consumer_init(&consumer, concat_consume_function, &consumer_opts, &concated_str));
    ASSERT_EQ(LEDGER_OK, ledger_consumer_attach(&consumer, &ctx, TOPIC, 0));
    EXPECT_EQ(LEDGER_OK, ledger_consumer_start(&consumer, LEDGER_END));
    // Not sure the best way to 'park' on the end.
    sleep(1);

    ASSERT_EQ(LEDGER_OK, ledger_write_partition(&ctx, TOPIC, 0, (void *)"hello", 5, &status));
    ledger_consumer_wait_for_position(&consumer, status.message_id);

    ledger_consumer_stop(&consumer);
    EXPECT_EQ(1, ledger_consumer_position_get(&consumer.position));
    EXPECT_EQ("hello", concated_str);

    ledger_consumer_close(&consumer);
    ledger_close_context(&ctx);
    ASSERT_EQ(0, cleanup(WORKING_DIR));
}

}
