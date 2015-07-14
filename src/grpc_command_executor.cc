#include <iostream>

#include <grpc/grpc.h>
#include <grpc++/channel_interface.h>
#include <grpc++/client_context.h>

#include "proto/ledgerd.grpc.pb.h"
#include "command_executor.h"
#include "grpc_command_executor.h"


namespace ledgerd {
void GrpcCommandExecutor::Execute(std::unique_ptr<Command> cmd) {
    switch(cmd->type()) {
        case CommandType::UNKNOWN: {
            UnknownCommand* command = static_cast<UnknownCommand*>(cmd.get());
            execute_unknown(command);
        } break;
        case CommandType::PING: {
            PingCommand* command = static_cast<PingCommand*>(cmd.get());
            execute_ping(command);
        } break;
        case CommandType::OPEN_TOPIC: {
            OpenTopicCommand* command = static_cast<OpenTopicCommand*>(cmd.get());
            execute_open_topic(command);
        } break;
        case CommandType::WRITE_PARTITION: {
            WritePartitionCommand* command = static_cast<WritePartitionCommand*>(cmd.get());
            execute_write_partition(command);
        } break;
        case CommandType::READ_PARTITION: {
            ReadPartitionCommand* command = static_cast<ReadPartitionCommand*>(cmd.get());
            execute_read_partition(command);
        } break;
        default: {
            UnknownCommand unknown(cmd->common_opts(), "");
            execute_unknown(&unknown);
        } break;
    }
}

void GrpcCommandExecutor::execute_unknown(const UnknownCommand* cmd) {
    std::cout << "Executing unknown" << std::endl;
}

void GrpcCommandExecutor::execute_ping(const PingCommand* cmd) {
    std::cout << "Executing ping" << std::endl;
}

void GrpcCommandExecutor::execute_open_topic(const OpenTopicCommand* cmd) {
    std::cout << "Executing open topic" << std::endl;
}

void GrpcCommandExecutor::execute_write_partition(const WritePartitionCommand* cmd) {
    std::cout << "Executing write partition" << std::endl;    
}

void GrpcCommandExecutor::execute_read_partition(const ReadPartitionCommand* cmd) {
    std::cout << "Executing read partition" << std::endl;
}
}
