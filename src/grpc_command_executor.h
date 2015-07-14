#ifndef LEDGERD_GRPC_COMMAND_EXECUTOR_H
#define LEDGERD_GRPC_COMMAND_EXECUTOR_H

#include <memory>

namespace ledgerd {
class GrpcCommandExecutor : public CommandExecutor {
    void execute_unknown(const UnknownCommand* cmd);
    void execute_ping(const PingCommand* cmd);
    void execute_open_topic(const OpenTopicCommand* cmd);
    void execute_write_partition(const WritePartitionCommand* cmd);
    void execute_read_partition(const ReadPartitionCommand* cmd);
public:
    void Execute(std::unique_ptr<Command> cmd);
};
}

#endif
