#include <sstream>

#include <grpc/grpc.h>
#include <grpc++/channel_interface.h>
#include <grpc++/client_context.h>

#include <grpc++/channel_arguments.h>
#include <grpc++/credentials.h>
#include <grpc++/create_channel.h>


#include "proto/ledgerd.grpc.pb.h"

#include "command_executor.h"
#include "grpc_command_executor.h"


namespace ledgerd {

std::unique_ptr<CommandExecutorStatus> GrpcCommandExecutor::Execute(std::unique_ptr<Command> cmd) {
    std::unique_ptr<Ledgerd::Stub> stub = connect(cmd->common_opts());
    switch(cmd->type()) {
        case CommandType::UNKNOWN: {
            UnknownCommand* command = static_cast<UnknownCommand*>(cmd.get());
            return execute_unknown(stub.get(), command);
        } break;
        case CommandType::PING: {
            PingCommand* command = static_cast<PingCommand*>(cmd.get());
            return execute_ping(stub.get(), command);
        } break;
        case CommandType::OPEN_TOPIC: {
            OpenTopicCommand* command = static_cast<OpenTopicCommand*>(cmd.get());
            return execute_open_topic(stub.get(), command);
        } break;
        case CommandType::WRITE_PARTITION: {
            WritePartitionCommand* command = static_cast<WritePartitionCommand*>(cmd.get());
            return execute_write_partition(stub.get(), command);
        } break;
        case CommandType::READ_PARTITION: {
            ReadPartitionCommand* command = static_cast<ReadPartitionCommand*>(cmd.get());
            return execute_read_partition(stub.get(), command);
        } break;
        default: {
            UnknownCommand unknown(cmd->common_opts(), "");
            return execute_unknown(stub.get(), &unknown);
        } break;
    }

    return std::unique_ptr<CommandExecutorStatus>(
        new CommandExecutorStatus {
            .code = CommandExecutorCode::OK,
                .lines = { "OK!" },
        });
}

std::unique_ptr<Ledgerd::Stub> GrpcCommandExecutor::connect(const CommonOptions& opts) {
    std::stringstream host_and_port;
    host_and_port << opts.host << ":" << opts.port;
    return Ledgerd::NewStub(
        grpc::CreateChannel(host_and_port.str(),
                            grpc::InsecureCredentials(),
                            grpc::ChannelArguments()));
}

std::unique_ptr<CommandExecutorStatus> GrpcCommandExecutor::execute_unknown(Ledgerd::Stub* stub, const UnknownCommand* cmd) {
    return std::unique_ptr<CommandExecutorStatus>(
        new CommandExecutorStatus {
            .code = CommandExecutorCode::OK,
                .lines = { "Unknown OK!" }});
}

std::unique_ptr<CommandExecutorStatus> GrpcCommandExecutor::execute_ping(Ledgerd::Stub* stub, const PingCommand* cmd) {
    return std::unique_ptr<CommandExecutorStatus>(
        new CommandExecutorStatus {
            .code = CommandExecutorCode::OK,
                .lines = { "Ping OK!" }});
}

std::unique_ptr<CommandExecutorStatus> GrpcCommandExecutor::execute_open_topic(Ledgerd::Stub* stub, const OpenTopicCommand* cmd) {
    return std::unique_ptr<CommandExecutorStatus>(
        new CommandExecutorStatus {
            .code = CommandExecutorCode::OK,
                .lines = { "Open Topic OK!" }});
}

std::unique_ptr<CommandExecutorStatus> GrpcCommandExecutor::execute_write_partition(Ledgerd::Stub* stub, const WritePartitionCommand* cmd) {
    return std::unique_ptr<CommandExecutorStatus>(
        new CommandExecutorStatus {
            .code = CommandExecutorCode::OK,
                .lines = { "Write partition OK!" }});
}

std::unique_ptr<CommandExecutorStatus> GrpcCommandExecutor::execute_read_partition(Ledgerd::Stub* stub, const ReadPartitionCommand* cmd) {
    return std::unique_ptr<CommandExecutorStatus>(
        new CommandExecutorStatus {
            .code = CommandExecutorCode::OK,
                .lines = { "Read partition OK!" }});
}
}
