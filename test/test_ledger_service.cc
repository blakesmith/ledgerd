#include <gtest/gtest.h>

#include "ledgerd_service.h"

namespace ledger_service_test {

using namespace ledgerd;

TEST(LedgerService, Initialization) {
    LedgerdServiceConfig config;
    config.set_root_directory("/tmp/ledgerd");

    LedgerdService ledger_service(config);
}
}
