#include <gtest/gtest.h>

#include <memory>
#include <thread>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_credentials.h>
#include <grpc++/channel_arguments.h>
#include <grpc++/credentials.h>
#include <grpc++/create_channel.h>

#include "ledgerd_service.h"
#include "ledgerd_service_config.h"
#include "grpc_interface.h"

namespace ledger_grpc_interface_test {

using namespace ledgerd;

static std::unique_ptr<Ledgerd::Stub> build_client() {
    return Ledgerd::NewStub(
        grpc::CreateChannel("localhost:50051",
                            grpc::InsecureCredentials(),
                            grpc::ChannelArguments()));
}

TEST(GrpcInterface, PingPong) {
    LedgerdServiceConfig config;
    config.set_grpc_address("0.0.0.0:50051");
    config.set_root_directory("/tmp/ledgerd");
    LedgerdService ledgerd_service(config);
    GrpcInterface grpc_interface(ledgerd_service);;
    grpc::ServerBuilder builder;
    builder.AddListeningPort(config.get_grpc_address(), grpc::InsecureServerCredentials());
    builder.RegisterService(&grpc_interface);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    auto client = build_client();
    PingRequest request;
    PingResponse response;
    request.set_ping("ping");
    grpc::ClientContext context;
    
    grpc::Status status = client->Ping(&context, request, &response);
    ASSERT_EQ(true, status.ok());
    EXPECT_EQ("pong", response.pong());

    server->Shutdown();
}

TEST(GrpcInterface, SimpleReadWrite) {
    LedgerdServiceConfig config;
    config.set_grpc_address("0.0.0.0:50051");
    config.set_root_directory("/tmp/ledgerd");
    LedgerdService ledgerd_service(config);
    GrpcInterface grpc_interface(ledgerd_service);;
    grpc::ServerBuilder builder;
    builder.AddListeningPort(config.get_grpc_address(), grpc::InsecureServerCredentials());
    builder.RegisterService(&grpc_interface);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    auto client = build_client();

    OpenTopicRequest treq;
    LedgerdResponse tres;
    treq.set_name("grpc_interface_topic");
    treq.set_partition_count(1);

    grpc::ClientContext ocontext;
    grpc::Status ostatus = client->OpenTopic(&ocontext, treq, &tres);
    ASSERT_EQ(true, ostatus.ok());
    ASSERT_EQ(LedgerdStatus::OK, tres.status());

    WritePartitionRequest wreq;
    WriteResponse wres;
    wreq.set_topic_name("grpc_interface_topic");
    wreq.set_partition_num(0);
    wreq.set_data("hello");
    grpc::ClientContext wcontext;
    grpc::Status wstatus = client->WritePartition(&wcontext, wreq, &wres);
    ASSERT_EQ(LedgerdStatus::OK, wres.ledger_response().status());
    ASSERT_EQ(true, wstatus.ok());

    uint64_t message_id = wres.message_id();
    
    ReadPartitionRequest rreq;
    ReadResponse rres;
    rreq.set_topic_name("grpc_interface_topic");
    rreq.set_partition_num(0);
    rreq.set_nmessages(1);
    rreq.set_start_id(message_id);
    grpc::ClientContext rcontext;
    grpc::Status rstatus = client->ReadPartition(&rcontext, rreq, &rres);
    ASSERT_EQ(LedgerdStatus::OK, rres.ledger_response().status());
    ASSERT_EQ(true, rstatus.ok());
    ASSERT_EQ(true, rres.has_messages());

    const LedgerdMessageSet& messages = rres.messages();
    ASSERT_EQ(1, messages.messages_size());
    const LedgerdMessage& message = messages.messages(0);
    EXPECT_EQ(message_id, message.id());
    EXPECT_EQ("hello", message.data());

    server->Shutdown();
}

TEST(GrpcInterface, StreamPartition) {
    LedgerdServiceConfig config;
    config.set_grpc_address("0.0.0.0:50051");
    config.set_root_directory("/tmp/ledgerd");
    LedgerdService ledgerd_service(config);
    GrpcInterface grpc_interface(ledgerd_service);;
    grpc::ServerBuilder builder;
    builder.AddListeningPort(config.get_grpc_address(), grpc::InsecureServerCredentials());
    builder.RegisterService(&grpc_interface);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    auto client = build_client();

    OpenTopicRequest treq;
    LedgerdResponse tres;
    treq.set_name("grpc_interface_topic");
    treq.set_partition_count(1);

    grpc::ClientContext ocontext;
    grpc::Status ostatus = client->OpenTopic(&ocontext, treq, &tres);
    ASSERT_EQ(true, ostatus.ok());
    ASSERT_EQ(LedgerdStatus::OK, tres.status());

    WritePartitionRequest wreq;
    WriteResponse wres;
    wreq.set_topic_name("grpc_interface_topic");
    wreq.set_partition_num(0);
    wreq.set_data("hello");
    grpc::ClientContext wcontext;
    grpc::Status wstatus = client->WritePartition(&wcontext, wreq, &wres);
    ASSERT_EQ(LedgerdStatus::OK, wres.ledger_response().status());
    ASSERT_EQ(true, wstatus.ok());

    uint64_t message_id = wres.message_id();

    StreamPartitionRequest sreq;
    LedgerdMessageSet messages;
    PositionSettings* position_settings = sreq.mutable_position_settings();
    position_settings->set_behavior(PositionBehavior::FORGET);
    sreq.set_topic_name("grpc_interface_topic");
    sreq.set_partition_num(0);
    sreq.set_start_id(message_id);
    sreq.set_read_chunk_size(64);

    grpc::ClientContext rcontext;
    std::unique_ptr<grpc::ClientReader<LedgerdMessageSet>> reader(client->StreamPartition(&rcontext, sreq));

    ASSERT_EQ(true, reader->Read(&messages));
    rcontext.TryCancel();
    grpc::Status sstatus = reader->Finish();

    ASSERT_EQ(grpc::StatusCode::CANCELLED, sstatus.error_code());
    ASSERT_EQ(1, messages.messages_size());

    const LedgerdMessage& message = messages.messages(0);
    EXPECT_EQ(message_id, message.id());
    EXPECT_EQ("hello", message.data());

    server->Shutdown();
}

}
