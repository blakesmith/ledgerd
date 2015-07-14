#include <iostream>
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
void GrpcCommandExecutor::Execute(std::unique_ptr<Command> cmd) {
    std::unique_ptr<Ledgerd::Stub> stub = connect(cmd->common_opts());
    switch(cmd->type()) {
        case CommandType::UNKNOWN: {
            UnknownCommand* command = static_cast<UnknownCommand*>(cmd.get());
            execute_unknown(stub.get(), command);
        } break;
        case CommandType::PING: {
            PingCommand* command = static_cast<PingCommand*>(cmd.get());
            execute_ping(stub.get(), command);
        } break;
        case CommandType::OPEN_TOPIC: {
            OpenTopicCommand* command = static_cast<OpenTopicCommand*>(cmd.get());
            execute_open_topic(stub.get(), command);
        } break;
        case CommandType::WRITE_PARTITION: {
            WritePartitionCommand* command = static_cast<WritePartitionCommand*>(cmd.get());
            execute_write_partition(stub.get(), command);
        } break;
        case CommandType::READ_PARTITION: {
            ReadPartitionCommand* command = static_cast<ReadPartitionCommand*>(cmd.get());
            execute_read_partition(stub.get(), command);
        } break;
        default: {
            UnknownCommand unknown(cmd->common_opts(), "");
            execute_unknown(stub.get(), &unknown);
        } break;
    }
}

std::unique_ptr<Ledgerd::Stub> GrpcCommandExecutor::connect(const CommonOptions& opts) {
    std::stringstream host_and_port;
    host_and_port << opts.host << ":" << opts.port;
    return Ledgerd::NewStub(
        grpc::CreateChannel(host_and_port.str(),
                            grpc::InsecureCredentials(),
                            grpc::ChannelArguments()));
}

void GrpcCommandExecutor::execute_unknown(Ledgerd::Stub* stub, const UnknownCommand* cmd) {
    std::cout << "Executing unknown" << std::endl;
}

void GrpcCommandExecutor::execute_ping(Ledgerd::Stub* stub, const PingCommand* cmd) {
    std::cout << "Executing ping" << std::endl;
}

void GrpcCommandExecutor::execute_open_topic(Ledgerd::Stub* stub, const OpenTopicCommand* cmd) {
    std::cout << "Executing open topic" << std::endl;
}

void GrpcCommandExecutor::execute_write_partition(Ledgerd::Stub* stub, const WritePartitionCommand* cmd) {
    std::cout << "Executing write partition" << std::endl;    
}

void GrpcCommandExecutor::execute_read_partition(Ledgerd::Stub* stub, const ReadPartitionCommand* cmd) {
    std::cout << "Executing read partition" << std::endl;
}
}
