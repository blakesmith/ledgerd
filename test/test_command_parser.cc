#include <gtest/gtest.h>

#include "command_executor.h"
#include "command_parser.h"

namespace ledger_command_parser_test {

using namespace ledgerd;

TEST(CommandParser, Ping) {
    CommandParser parser;
    char *argv[] { "ledgerd_client", "ping" };
    int argc = 2;

    auto command = parser.MakeCommand(argv, argc);
    EXPECT_EQ("ping", command->name());
    EXPECT_EQ(CommandType::PING, command->type());
}

}
