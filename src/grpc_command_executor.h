#ifndef LEDGERD_GRPC_COMMAND_EXECUTOR_H
#define LEDGERD_GRPC_COMMAND_EXECUTOR_H

#include "proto/ledgerd.grpc.pb.h"
#include "command_executor.h"

#include <memory>

namespace ledgerd {
class GrpcCommandExecutor : public CommandExecutor {
    std::unique_ptr<Ledgerd::Stub> connect(const CommonOptions& opts);
    std::unique_ptr<CommandExecutorStatus> execute_unknown(Ledgerd::Stub* stub, const UnknownCommand* cmd);
    std::unique_ptr<CommandExecutorStatus> execute_ping(Ledgerd::Stub* stub, const PingCommand* cmd);
    std::unique_ptr<CommandExecutorStatus> execute_open_topic(Ledgerd::Stub* stub, const OpenTopicCommand* cmd);
    std::unique_ptr<CommandExecutorStatus> execute_write_partition(Ledgerd::Stub* stub, const WritePartitionCommand* cmd);
    std::unique_ptr<CommandExecutorStatus> execute_read_partition(Ledgerd::Stub* stub, const ReadPartitionCommand* cmd);
public:
    std::unique_ptr<CommandExecutorStatus> Execute(std::unique_ptr<Command> cmd);
};
}

#endif
