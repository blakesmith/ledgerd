#include <sstream>

#include <grpc/grpc.h>
#include <grpc++/channel_interface.h>
#include <grpc++/client_context.h>

#include <grpc++/channel_arguments.h>
#include <grpc++/credentials.h>
#include <grpc++/create_channel.h>

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
    PingRequest request;
    PingResponse response;
    request.set_ping("ping");
    grpc::ClientContext context;
    std::unique_ptr<CommandExecutorStatus> exec_status(
        new CommandExecutorStatus());

    grpc::Status status = stub->Ping(&context, request, &response);
    if(status.ok()) {
        exec_status->code = CommandExecutorCode::OK;
        exec_status->lines.push_back("Pong!");
        return exec_status;
    }

    exec_status->code = CommandExecutorCode::ERROR;
    exec_status->lines.push_back("Error pinging");
    exec_status->lines.push_back(status.error_message());
    return exec_status;
}

std::unique_ptr<CommandExecutorStatus> GrpcCommandExecutor::execute_open_topic(Ledgerd::Stub* stub, const OpenTopicCommand* cmd) {
    OpenTopicRequest request;
    LedgerdResponse response;
    request.set_name(cmd->topic_name());
    request.set_partition_count(cmd->partition_count());
    grpc::ClientContext context;
    std::unique_ptr<CommandExecutorStatus> exec_status(
        new CommandExecutorStatus());

    grpc::Status status = stub->OpenTopic(&context, request, &response);

    if(status.ok()) {
        if(response.status() == LedgerdStatus::OK) {
            std::stringstream ss;
            ss << "OPENED: " << cmd->topic_name();
            exec_status->code = CommandExecutorCode::OK;
            exec_status->lines.push_back(ss.str());
        } else {
            std::stringstream ss;
            ss << "Ledger code: " << response.status();
            exec_status->code = CommandExecutorCode::ERROR;
            exec_status->lines.push_back("Ledger error when wrting to partition");
            exec_status->lines.push_back(response.error_message());
            exec_status->lines.push_back(ss.str());
        }

        return exec_status;
    }

    exec_status->code = CommandExecutorCode::ERROR;
    exec_status->lines.push_back("Error opening topic");
    exec_status->lines.push_back(status.error_message());
    return exec_status;
}

std::unique_ptr<CommandExecutorStatus> GrpcCommandExecutor::execute_write_partition(Ledgerd::Stub* stub, const WritePartitionCommand* cmd) {
    WritePartitionRequest request;
    WriteResponse response;
    request.set_topic_name(cmd->topic_name());
    request.set_partition_num(cmd->partition_num());
    request.set_data(cmd->data());
    grpc::ClientContext context;
    std::unique_ptr<CommandExecutorStatus> exec_status(
        new CommandExecutorStatus());

    grpc::Status status = stub->WritePartition(&context, request, &response);
    if(status.ok()) {
        const LedgerdResponse& lresponse = response.ledger_response();
        if(lresponse.status() == LedgerdStatus::OK) {
            std::stringstream ss;
            ss << "ID: " << response.message_id();
            exec_status->code = CommandExecutorCode::OK;
            exec_status->lines.push_back(ss.str());
        } else {
            std::stringstream ss;
            ss << "Ledger code: " << lresponse.status();
            exec_status->code = CommandExecutorCode::ERROR;
            exec_status->lines.push_back("Ledger error when wrting to partition");
            exec_status->lines.push_back(lresponse.error_message());
            exec_status->lines.push_back(ss.str());
        }

        return exec_status;
    }

    exec_status->code = CommandExecutorCode::ERROR;
    exec_status->lines.push_back("Grpc error writing to partition");
    exec_status->lines.push_back(status.error_message());
    return exec_status;
}

std::unique_ptr<CommandExecutorStatus> GrpcCommandExecutor::execute_read_partition(Ledgerd::Stub* stub, const ReadPartitionCommand* cmd) {
    ReadPartitionRequest request;
    ReadResponse response;
    request.set_topic_name(cmd->topic_name());
    request.set_partition_num(cmd->partition_num());
    request.set_start_id(cmd->start_id());
    request.set_nmessages(cmd->nmessages());
    grpc::ClientContext context;
    std::unique_ptr<CommandExecutorStatus> exec_status(
        new CommandExecutorStatus());
    
    grpc::Status status = stub->ReadPartition(&context, request, &response);
    if(status.ok()) {
        const LedgerdResponse& lresponse = response.ledger_response();
        if(lresponse.status() == LedgerdStatus::OK) {
            exec_status->code = CommandExecutorCode::OK;
            const LedgerdMessageSet& messages = response.messages();
            std::stringstream ss;
            ss << "Next message ID: " << messages.next_id();
            exec_status->lines.push_back(ss.str());
            
            for(int i = 0; i < messages.messages_size(); ++i) {
                const LedgerdMessage& message = messages.messages(i);
                exec_status->lines.push_back(message.data());
            }
        } else {
            std::stringstream ss;
            ss << "Ledger code: " << lresponse.status();
            exec_status->code = CommandExecutorCode::ERROR;
            exec_status->lines.push_back("Ledger error when reading from partition");
            exec_status->lines.push_back(lresponse.error_message());
            exec_status->lines.push_back(ss.str());
        }

        return exec_status;
    }
    exec_status->code = CommandExecutorCode::ERROR;
    exec_status->lines.push_back("Grpc error when reading partition");
    exec_status->lines.push_back(status.error_message());
    return exec_status;
}
}
