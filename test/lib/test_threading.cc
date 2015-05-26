#include <gtest/gtest.h>

#include <pthread.h>
#include <sys/stat.h>
#include <ftw.h>

#include "ledger.h"

namespace ledger_threading_test {
static const char *WORKING_DIR = "/tmp/ledger_threading";
static const char *TOPIC = "threaded_data";

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

static const int NUM_THREADS = 2;
static const int NUM_MESSAGES = 10000;

void *write_worker(void *ctx_ptr) {
    ledger_ctx *ctx = (ledger_ctx *)ctx_ptr;
    int i;
    ledger_status rc;
    uint32_t message = 1;

    for(i = 0; i < NUM_MESSAGES; i++) {
        rc = ledger_write_partition(ctx, TOPIC, 0, (void *)&message, sizeof(uint32_t));
        if(rc != LEDGER_OK) {
            printf("Failed to write to partition: %d\n", rc);
            exit(rc);
        }
    }

    return NULL;
}

TEST(LedgerThreading, WriteThreading) {
    ledger_ctx ctx;
    pthread_t threads[NUM_THREADS];
    int i, count;
    ledger_message_set messages;
    uint32_t read_message;

    cleanup(WORKING_DIR);
    ASSERT_EQ(0, setup(WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_context(&ctx, WORKING_DIR));
    ASSERT_EQ(LEDGER_OK, ledger_open_topic(&ctx, TOPIC, 1, 0));

    for(i = 0; i < NUM_THREADS; i++) {
        ASSERT_EQ(0, pthread_create(&threads[i], NULL, write_worker, &ctx));
    }

    for(i = 0; i < NUM_THREADS; i++) {
        ASSERT_EQ(0, pthread_join(threads[i], NULL));
    }

    ASSERT_EQ(LEDGER_OK, ledger_read_partition(&ctx, TOPIC, 0, LEDGER_BEGIN, NUM_THREADS * NUM_MESSAGES, &messages));
    EXPECT_EQ(NUM_THREADS * NUM_MESSAGES, messages.nmessages);

    count = 0;
    for(i = 0; i < messages.nmessages; i++) {
        read_message = *(int *)messages.messages[i].data;
        count = count + read_message;
    }
    EXPECT_EQ(messages.nmessages, count);

    ledger_message_set_free(&messages);
    ledger_close_context(&ctx);
    ASSERT_EQ(0, cleanup(WORKING_DIR));
}
}
