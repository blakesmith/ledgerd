#include <gtest/gtest.h>

#include "ledgerd_service.h"

namespace ledger_service_test {

using namespace ledgerd;

TEST(LedgerService, SimpleWriteRead) {
    ledger_status rc;
    ledger_topic_options topic_options;

    LedgerdServiceConfig config;
    config.set_root_directory("/tmp/ledgerd");

    LedgerdService ledger_service(config);

    ledger_topic_options_init(&topic_options);
    rc = ledger_service.OpenTopic("my_topic", 1, &topic_options);
    ASSERT_EQ(LEDGER_OK, rc);
}
}
