#include <gtest/gtest.h>

#include "command_executor.h"
#include "command_parser.h"

namespace ledger_command_parser_test {

using namespace ledgerd;

TEST(CommandParser, Ping) {
    CommandParser parser;
    const char *argv[] { "ledgerd_client", "--command", "ping" };
    int argc = 3;

    auto command = parser.MakeCommand(const_cast<char**>(argv), argc);
    EXPECT_EQ("ping", command->name());
    EXPECT_EQ(CommandType::PING, command->type());
}

TEST(CommandParser, OpenTopic) {
    CommandParser parser;
    const char *argv[] { "ledgerd_client", "--command", "open_topic", "--topic", "my_topic", "--partition_count", "1" };
    int argc = 7;

    auto command = parser.MakeCommand(const_cast<char**>(argv), argc);
    EXPECT_EQ("open_topic", command->name());
    EXPECT_EQ(CommandType::OPEN_TOPIC, command->type());
    OpenTopicCommand* cmd = static_cast<OpenTopicCommand*>(command.get());
    EXPECT_EQ("my_topic", cmd->topic_name());
    EXPECT_EQ(1, cmd->partition_count());
}

}
