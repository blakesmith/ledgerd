#include <sstream>
#include <iostream>

#include <grpc/grpc.h>
#include <grpc++/channel.h>
#include <grpc++/client_context.h>

#include <grpc++/security/credentials.h>
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
        case CommandType::GET_TOPIC: {
            GetTopicCommand* command = static_cast<GetTopicCommand*>(cmd.get());
            return execute_get_topic(stub.get(), command);
        } break;
        case CommandType::WRITE_PARTITION: {
            WritePartitionCommand* command = static_cast<WritePartitionCommand*>(cmd.get());
            return execute_write_partition(stub.get(), command);
        } break;
        case CommandType::READ_PARTITION: {
            ReadPartitionCommand* command = static_cast<ReadPartitionCommand*>(cmd.get());
            return execute_read_partition(stub.get(), command);
        } break;
        case CommandType::STREAM_PARTITION: {
            StreamPartitionCommand* command = static_cast<StreamPartitionCommand*>(cmd.get());
            return execute_stream_partition(std::move(stub), command);
        } break;
        case CommandType::STREAM: {
            StreamCommand* command = static_cast<StreamCommand*>(cmd.get());
            return execute_stream(std::move(stub), command);
        } break;
        default: {
            UnknownCommand unknown(cmd->common_opts(), "");
            return execute_unknown(stub.get(), &unknown);
        } break;
    }

    std::unique_ptr<CommandExecutorStatus> exec_status(
        new CommandExecutorStatus(CommandExecutorCode::OK));
    exec_status->AddLine("OK!");
    exec_status->Close();
    return exec_status;
}

void GrpcCommandExecutor::Stop() {
    if(stream_thread.joinable()) {
        stream_thread.join();
    }
}

std::unique_ptr<Ledgerd::Stub> GrpcCommandExecutor::connect(const CommonOptions& opts) {
    std::stringstream host_and_port;
    host_and_port << opts.host << ":" << opts.port;
    return Ledgerd::NewStub(
        grpc::CreateChannel(host_and_port.str(),
                            grpc::InsecureChannelCredentials()));
}

std::unique_ptr<CommandExecutorStatus> GrpcCommandExecutor::execute_unknown(Ledgerd::Stub* stub, const UnknownCommand* cmd) {
    std::unique_ptr<CommandExecutorStatus> exec_status(
        new CommandExecutorStatus());
    if(cmd->command_name() == "") {
        exec_status->set_code(CommandExecutorCode::OK);
    } else {
        std::stringstream ss;
        ss << "Unknown command: " << cmd->command_name();
        exec_status->set_code(CommandExecutorCode::ERROR);
        exec_status->AddLine(ss.str());
    }
    exec_status->AddLine("Usage: ledger_client [ options ...]");
    exec_status->AddLine("    -H --help            Print this message and exit.");
    exec_status->AddLine("    -h --host            Ledgerd hostname. Defaults to: 'localhost'");
    exec_status->AddLine("    -p --port            Ledgerd port. Defaults to: 64399");
    exec_status->AddLine("    -c --command         Ledgerd command.");
    exec_status->AddLine("    -t --topic           The topic to read or write from.");
    exec_status->AddLine("    -C --partition_count Number of partitions.");
    exec_status->AddLine("    -P --partition       The partition to read or write from.");
    exec_status->AddLine("    -d --data            The data to write.");
    exec_status->AddLine("    -s --start           The start_id to begin reading from, defaults to: 0.");
    exec_status->AddLine("    -n --nmessages       Number of messages to read. Defaults to: 1");
    exec_status->Close();

    return exec_status;
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
        exec_status->set_code(CommandExecutorCode::OK);
        exec_status->AddLine("Pong!");
        exec_status->Close();
        return exec_status;
    }

    exec_status->set_code(CommandExecutorCode::ERROR);
    exec_status->AddLine("Error pinging");
    exec_status->AddLine(status.error_message());
    exec_status->Close();
    return exec_status;
}

std::unique_ptr<CommandExecutorStatus> GrpcCommandExecutor::execute_open_topic(Ledgerd::Stub* stub, const OpenTopicCommand* cmd) {
    OpenTopicRequest request;
    LedgerdResponse response;
    request.set_name(cmd->topic_name());
    for(int i = 0; i < cmd->partition_count(); i++) {
        request.add_partition_ids(i);
    }
    grpc::ClientContext context;
    std::unique_ptr<CommandExecutorStatus> exec_status(
        new CommandExecutorStatus());

    grpc::Status status = stub->OpenTopic(&context, request, &response);

    if(status.ok()) {
        if(response.status() == LedgerdStatus::OK) {
            std::stringstream ss;
            ss << "OPENED: " << cmd->topic_name();
            exec_status->set_code(CommandExecutorCode::OK);
            exec_status->AddLine(ss.str());
        } else {
            std::stringstream ss;
            ss << "Ledger code: " << response.status();
            exec_status->set_code(CommandExecutorCode::ERROR);
            exec_status->AddLine("Ledger error when opening topic");
            exec_status->AddLine(response.error_message());
            exec_status->AddLine(ss.str());
        }

        exec_status->Close();
        return exec_status;
    }

    exec_status->set_code(CommandExecutorCode::ERROR);
    exec_status->AddLine("Error opening topic");
    exec_status->AddLine(status.error_message());
    exec_status->Close();
    return exec_status;
}

std::unique_ptr<CommandExecutorStatus> GrpcCommandExecutor::execute_get_topic(Ledgerd::Stub* stub, const GetTopicCommand* cmd) {
    TopicRequest request;
    TopicResponse response;
    request.set_topic_name(cmd->topic_name());
    grpc::ClientContext context;
    std::unique_ptr<CommandExecutorStatus> exec_status(
        new CommandExecutorStatus());

    grpc::Status status = stub->GetTopic(&context, request, &response);
    if(status.ok()) {
        std::stringstream ss;
        if(response.found()) {
            ss << "Name: " << response.topic().name() << std::endl;
            ss << "Partition count: " << response.topic().npartitions();
            exec_status->AddLine(ss.str());
        } else {
            ss << "Topic not found: " << cmd->topic_name();
            exec_status->AddLine(ss.str());
        }

        exec_status->Close();
        return exec_status;
    }

    exec_status->set_code(CommandExecutorCode::ERROR);
    exec_status->AddLine("Grpc error when looking up topic");
    exec_status->AddLine(status.error_message());
    exec_status->Close();
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
            exec_status->set_code(CommandExecutorCode::OK);
            exec_status->AddLine(ss.str());
        } else {
            std::stringstream ss;
            ss << "Ledger code: " << lresponse.status();
            exec_status->set_code(CommandExecutorCode::ERROR);
            exec_status->AddLine("Ledger error when writing to partition");
            exec_status->AddLine(lresponse.error_message());
            exec_status->AddLine(ss.str());
        }

        exec_status->Close();
        return exec_status;
    }

    exec_status->set_code(CommandExecutorCode::ERROR);
    exec_status->AddLine("Grpc error writing to partition");
    exec_status->AddLine(status.error_message());
    exec_status->Close();
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
            exec_status->set_code(CommandExecutorCode::OK);
            const LedgerdMessageSet& messages = response.messages();
            std::stringstream ss;
            ss << "Next message ID: " << messages.next_id();
            exec_status->AddLine(ss.str());
            
            for(int i = 0; i < messages.messages_size(); ++i) {
                const LedgerdMessage& message = messages.messages(i);
                exec_status->AddLine(message.data());
            }
        } else {
            std::stringstream ss;
            ss << "Ledger code: " << lresponse.status();
            exec_status->set_code(CommandExecutorCode::ERROR);
            exec_status->AddLine("Ledger error when reading from partition");
            exec_status->AddLine(lresponse.error_message());
            exec_status->AddLine(ss.str());
        }

        exec_status->Close();
        return exec_status;
    }
    exec_status->set_code(CommandExecutorCode::ERROR);
    exec_status->AddLine("Grpc error when reading partition");
    exec_status->AddLine(status.error_message());
    exec_status->Close();
    return exec_status;
}

std::unique_ptr<CommandExecutorStatus> GrpcCommandExecutor::execute_stream_partition(std::unique_ptr<Ledgerd::Stub> stub, const StreamPartitionCommand* cmd) {
    std::unique_ptr<CommandExecutorStatus> exec_status(
        new CommandExecutorStatus(CommandExecutorCode::OK));
    stream_thread = std::thread(GrpcCommandExecutor::stream_partition_read,
                                std::move(stub),
                                cmd,
                                exec_status.get());
    return exec_status;
}

std::unique_ptr<CommandExecutorStatus> GrpcCommandExecutor::execute_stream(std::unique_ptr<Ledgerd::Stub> stub, const StreamCommand* cmd) {
    std::unique_ptr<CommandExecutorStatus> exec_status(
        new CommandExecutorStatus(CommandExecutorCode::OK));

    TopicRequest request;
    TopicResponse response;
    request.set_topic_name(cmd->topic_name());
    grpc::ClientContext context;
    grpc::Status status = stub->GetTopic(&context, request, &response);
    if(status.ok()) {
        if(response.found()) {
            const uint32_t npartitions = response.topic().npartitions();
            stream_thread = std::thread(GrpcCommandExecutor::stream_read,
                                        std::move(stub),
                                        cmd,
                                        npartitions,
                                        exec_status.get());
        } else {
            std::stringstream ss;
            ss << "Topic not found: " << cmd->topic_name();
            exec_status->AddLine(ss.str());
            exec_status->Close();
        }

        return exec_status;
    }

    exec_status->set_code(CommandExecutorCode::ERROR);
    exec_status->AddLine("Grpc error when streaming read");
    exec_status->AddLine(status.error_message());
    exec_status->Close();
    return exec_status;
}

void GrpcCommandExecutor::stream_partition_read(std::unique_ptr<Ledgerd::Stub> stub,
                                                const StreamPartitionCommand* cmd,
                                                CommandExecutorStatus* exec_status) {

    StreamPartitionRequest sreq;
    LedgerdMessageSet messages;
    PositionSettings* position_settings = sreq.mutable_position_settings();
    position_settings->set_behavior(PositionBehavior::FORGET);
    sreq.set_topic_name(cmd->topic_name());
    sreq.set_partition_num(cmd->partition_num());
    sreq.set_start_id(cmd->start_id());
    sreq.set_read_chunk_size(64);

    grpc::ClientContext rcontext;
    std::unique_ptr<grpc::ClientReader<LedgerdMessageSet>> reader(stub->StreamPartition(&rcontext, sreq));

    while(reader->Read(&messages)) {
        for(int i = 0; i < messages.messages_size(); i++) {
            const LedgerdMessage& message = messages.messages(i);
            exec_status->AddLine(message.data());
        }
        exec_status->Flush();
    }
    reader->Finish();
    exec_status->Close();
}

void GrpcCommandExecutor::stream_read(std::unique_ptr<Ledgerd::Stub> stub,
                                      const StreamCommand* cmd,
                                      uint32_t npartitions,
                                      CommandExecutorStatus* exec_status) {

    StreamRequest sreq;
    LedgerdMessageSet messages;
    PositionSettings* position_settings = sreq.mutable_position_settings();
    // TODO: Should we support remembering positions?
    position_settings->set_behavior(PositionBehavior::FORGET);
    // Request all the partitions
    for(int i = 0; i < npartitions; i++) sreq.add_partition_ids(i);
    sreq.set_topic_name(cmd->topic_name());
    sreq.set_read_chunk_size(64);

    grpc::ClientContext rcontext;
    std::unique_ptr<grpc::ClientReader<LedgerdMessageSet>> reader(stub->Stream(&rcontext, sreq));

    while(reader->Read(&messages)) {
        for(int i = 0; i < messages.messages_size(); i++) {
            const LedgerdMessage& message = messages.messages(i);
            exec_status->AddLine(message.data());
        }
        exec_status->Flush();
    }
    reader->Finish();
    exec_status->Close();
}
}



