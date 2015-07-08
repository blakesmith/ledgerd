#include <gtest/gtest.h>

#include "ledgerd_service.h"

namespace ledger_service_test {

using namespace ledgerd;

TEST(LedgerService, SimpleWriteRead) {
    ledger_status rc;
    ledger_write_status write_status;
    ledger_topic_options topic_options;
    ledger_message_set messages;

    LedgerdServiceConfig config;
    config.set_root_directory("/tmp/ledgerd");

    LedgerdService ledger_service(config);

    ledger_topic_options_init(&topic_options);
    rc = ledger_service.OpenTopic("my_topic", 1, &topic_options);
    ASSERT_EQ(LEDGER_OK, rc);
    rc = ledger_service.WritePartition("my_topic", 0, "hello", &write_status);
    ASSERT_EQ(LEDGER_OK, rc);
    rc = ledger_service.ReadPartition("my_topic", 0, write_status.message_id, 1, &messages);
    ASSERT_EQ(LEDGER_OK, rc);
    EXPECT_EQ(1, messages.nmessages);
    ledger_message_set_free(&messages);
}
}
