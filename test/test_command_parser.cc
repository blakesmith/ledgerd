#include <gtest/gtest.h>

#include "command_executor.h"
#include "command_parser.h"

namespace ledger_command_parser_test {

using namespace ledgerd;

TEST(CommandParser, Ping) {
    CommandParser parser;
    const char *argv[] { "ledgerd_client", "--command", "ping" };
    int argc = 3;

    auto command = parser.MakeCommand(argc, const_cast<char**>(argv));
    EXPECT_EQ("ping", command->name());
    EXPECT_EQ(CommandType::PING, command->type());
}

TEST(CommandParser, CommonOptions) {
    CommandParser parser;
    const char *argv[] { "ledgerd_client", "--command", "ping", "--host", "blah.com", "--port", "6789" };
    int argc = 7;

    auto command = parser.MakeCommand(argc, const_cast<char**>(argv));
    EXPECT_EQ("blah.com", command->common_opts().host);
    EXPECT_EQ(6789, command->common_opts().port);
}

TEST(CommandParser, CommonOptionsDefaults) {
    CommandParser parser;
    const char *argv[] { "ledgerd_client", "--command", "ping" };
    int argc = 3;

    auto command = parser.MakeCommand(argc, const_cast<char**>(argv));
    EXPECT_EQ("localhost", command->common_opts().host);
    EXPECT_EQ(64399, command->common_opts().port);
}

TEST(CommandParser, OpenTopic) {
    CommandParser parser;
    const char *argv[] { "ledgerd_client", "--command", "open_topic", "--topic", "my_topic", "--partition_count", "1" };
    int argc = 7;

    auto command = parser.MakeCommand(argc, const_cast<char**>(argv));
    EXPECT_EQ("open_topic", command->name());
    ASSERT_EQ(CommandType::OPEN_TOPIC, command->type());
    OpenTopicCommand* cmd = static_cast<OpenTopicCommand*>(command.get());
    EXPECT_EQ("my_topic", cmd->topic_name());
    EXPECT_EQ(1, cmd->partition_count());
}

TEST(CommandParser, GetTopic) {
    CommandParser parser;
    const char *argv[] { "ledgerd_client", "--command", "get_topic", "--topic", "my_topic" };
    int argc = 5;

    auto command = parser.MakeCommand(argc, const_cast<char**>(argv));
    EXPECT_EQ("get_topic", command->name());
    ASSERT_EQ(CommandType::GET_TOPIC, command->type());
    GetTopicCommand* cmd = static_cast<GetTopicCommand*>(command.get());
    EXPECT_EQ("my_topic", cmd->topic_name());
}

TEST(CommandParser, WritePartition) {
    CommandParser parser;
    const char *argv[] { "ledgerd_client", "--command", "write_partition", "--topic", "my_topic", "--partition", "0", "--data", "hello" };
    int argc = 9;

    auto command = parser.MakeCommand(argc, const_cast<char**>(argv));
    EXPECT_EQ("write_partition", command->name());
    ASSERT_EQ(CommandType::WRITE_PARTITION, command->type());
    WritePartitionCommand* cmd = static_cast<WritePartitionCommand*>(command.get());
    EXPECT_EQ("my_topic", cmd->topic_name());
    EXPECT_EQ(0, cmd->partition_num());
    EXPECT_EQ("hello", cmd->data());
}

TEST(CommandParser, ReadPartition) {
    CommandParser parser;
    const char *argv[] { "ledgerd_client", "--command", "read_partition", "--topic", "my_topic", "--partition", "0", "--start", "42", "--nmessages", "2" };
    int argc = 11;

    auto command = parser.MakeCommand(argc, const_cast<char**>(argv));
    EXPECT_EQ("read_partition", command->name());
    ASSERT_EQ(CommandType::READ_PARTITION, command->type());
    ReadPartitionCommand* cmd = static_cast<ReadPartitionCommand*>(command.get());
    EXPECT_EQ("my_topic", cmd->topic_name());
    EXPECT_EQ(0, cmd->partition_num());
    EXPECT_EQ(42, cmd->start_id());
    EXPECT_EQ(2, cmd->nmessages());
}

TEST(CommandParser, StreamPartition) {
    CommandParser parser;
    const char *argv[] { "ledgerd_client", "--command", "stream_partition", "--topic", "my_topic", "--partition", "0", "--start", "42" };
    int argc = 9;

    auto command = parser.MakeCommand(argc, const_cast<char**>(argv));
    EXPECT_EQ("stream_partition", command->name());
    ASSERT_EQ(CommandType::STREAM_PARTITION, command->type());
    StreamPartitionCommand* cmd = static_cast<StreamPartitionCommand*>(command.get());
    EXPECT_EQ("my_topic", cmd->topic_name());
    EXPECT_EQ(0, cmd->partition_num());
    EXPECT_EQ(42, cmd->start_id());
}

TEST(CommandParser, Stream) {
    CommandParser parser;
    const char *argv[] { "ledgerd_client", "--command", "stream", "--topic", "my_topic" };
    int argc = 5;

    auto command = parser.MakeCommand(argc, const_cast<char**>(argv));
    EXPECT_EQ("stream", command->name());
    ASSERT_EQ(CommandType::STREAM, command->type());
    StreamCommand* cmd = static_cast<StreamCommand*>(command.get());
    EXPECT_EQ("my_topic", cmd->topic_name());
}

TEST(CommandParser, ReadPartitionNoStartOrNmessages) {
    CommandParser parser;
    const char *argv[] { "ledgerd_client", "--command", "read_partition", "--topic", "my_topic", "--partition", "0" };
    int argc = 7;

    auto command = parser.MakeCommand(argc, const_cast<char**>(argv));
    EXPECT_EQ("read_partition", command->name());
    ASSERT_EQ(CommandType::READ_PARTITION, command->type());
    ReadPartitionCommand* cmd = static_cast<ReadPartitionCommand*>(command.get());
    EXPECT_EQ("my_topic", cmd->topic_name());
    EXPECT_EQ(0, cmd->partition_num());
    EXPECT_EQ(0, cmd->start_id());
    EXPECT_EQ(1, cmd->nmessages());
}

TEST(CommandParser, UnknownCommand) {
    CommandParser parser;
    const char *argv[] { "ledgerd_client", "--command", "fake_command" };
    int argc = 3;

    auto command = parser.MakeCommand(argc, const_cast<char**>(argv));
    EXPECT_EQ("unknown", command->name());
    ASSERT_EQ(CommandType::UNKNOWN, command->type());
    UnknownCommand* cmd = static_cast<UnknownCommand*>(command.get());
    EXPECT_EQ("fake_command", cmd->command_name());
}

TEST(CommandParser, Help) {
    CommandParser parser;
    const char *argv[] { "ledgerd_client", "--help" };
    int argc = 3;

    auto command = parser.MakeCommand(argc, const_cast<char**>(argv));
    EXPECT_EQ("unknown", command->name());
    ASSERT_EQ(CommandType::UNKNOWN, command->type());
    UnknownCommand* cmd = static_cast<UnknownCommand*>(command.get());
    EXPECT_EQ("", cmd->command_name());
}

}
