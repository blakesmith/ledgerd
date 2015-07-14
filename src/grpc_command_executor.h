#ifndef LEDGERD_GRPC_COMMAND_EXECUTOR_H
#define LEDGERD_GRPC_COMMAND_EXECUTOR_H

#include <memory>

namespace ledgerd {
class GrpcCommandExecutor : public CommandExecutor {
    std::unique_ptr<Ledgerd::Stub> connect(const CommonOptions& opts);
    void execute_unknown(Ledgerd::Stub* stub, const UnknownCommand* cmd);
    void execute_ping(Ledgerd::Stub* stub, const PingCommand* cmd);
    void execute_open_topic(Ledgerd::Stub* stub, const OpenTopicCommand* cmd);
    void execute_write_partition(Ledgerd::Stub* stub, const WritePartitionCommand* cmd);
    void execute_read_partition(Ledgerd::Stub* stub, const ReadPartitionCommand* cmd);
public:
    void Execute(std::unique_ptr<Command> cmd);
};
}

#endif
