#ifndef LEDGERD_GRPC_COMMAND_EXECUTOR_H
#define LEDGERD_GRPC_COMMAND_EXECUTOR_H

#include "proto/ledgerd.grpc.pb.h"
#include "command_executor.h"

#include <memory>
#include <thread>

namespace ledgerd {
class GrpcCommandExecutor : public CommandExecutor {
    std::thread stream_thread;

    static void stream_partition_read(std::unique_ptr<Ledgerd::Stub> stub,
                                      const StreamPartitionCommand* cmd,
                                      CommandExecutorStatus* exec_status);
    
    static void stream_read(std::unique_ptr<Ledgerd::Stub> stub,
                            const StreamCommand* cmd,
                            uint32_t npartitions,
                            CommandExecutorStatus* exec_status);

    std::unique_ptr<Ledgerd::Stub> connect(const CommonOptions& opts);
    std::unique_ptr<CommandExecutorStatus> execute_unknown(Ledgerd::Stub* stub, const UnknownCommand* cmd);
    std::unique_ptr<CommandExecutorStatus> execute_ping(Ledgerd::Stub* stub, const PingCommand* cmd);
    std::unique_ptr<CommandExecutorStatus> execute_open_topic(Ledgerd::Stub* stub, const OpenTopicCommand* cmd);
    std::unique_ptr<CommandExecutorStatus> execute_get_topic(Ledgerd::Stub* stub, const GetTopicCommand* cmd);
    std::unique_ptr<CommandExecutorStatus> execute_write_partition(Ledgerd::Stub* stub, const WritePartitionCommand* cmd);
    std::unique_ptr<CommandExecutorStatus> execute_read_partition(Ledgerd::Stub* stub, const ReadPartitionCommand* cmd);
    std::unique_ptr<CommandExecutorStatus> execute_stream_partition(std::unique_ptr<Ledgerd::Stub> stub, const StreamPartitionCommand* cmd);
    std::unique_ptr<CommandExecutorStatus> execute_stream(std::unique_ptr<Ledgerd::Stub> stub, const StreamCommand* cmd);
public:
    std::unique_ptr<CommandExecutorStatus> Execute(std::unique_ptr<Command> cmd);
    void Stop();
};
}

#endif
