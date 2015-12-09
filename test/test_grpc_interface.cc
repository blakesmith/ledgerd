#include <gtest/gtest.h>

#include <memory>
#include <thread>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/security/server_credentials.h>
#include <grpc++/channel.h>
#include <grpc++/security/credentials.h>
#include <grpc++/create_channel.h>

#include "ledgerd_service.h"
#include "ledgerd_service_config.h"
#include "grpc_interface.h"

namespace ledger_grpc_interface_test {

using namespace ledgerd;

static std::unique_ptr<Ledgerd::Stub> build_client() {
    return Ledgerd::NewStub(
        grpc::CreateChannel("localhost:50051",
                            grpc::InsecureChannelCredentials()));
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
    treq.add_partition_ids(0);
    treq.add_partition_ids(1);

    grpc::ClientContext ocontext;
    grpc::Status ostatus = client->OpenTopic(&ocontext, treq, &tres);
    ASSERT_EQ(true, ostatus.ok());
    ASSERT_EQ(LedgerdStatus::OK, tres.status());

    WritePartitionRequest wreq;
    WriteResponse wres;
    wreq.set_topic_name("grpc_interface_topic");
    wreq.set_partition_num(1);
    wreq.set_data("hello");
    grpc::ClientContext wcontext;
    grpc::Status wstatus = client->WritePartition(&wcontext, wreq, &wres);
    ASSERT_EQ(LedgerdStatus::OK, wres.ledger_response().status());
    ASSERT_EQ(true, wstatus.ok());

    uint64_t message_id = wres.message_id();
    
    ReadPartitionRequest rreq;
    ReadResponse rres;
    rreq.set_topic_name("grpc_interface_topic");
    rreq.set_partition_num(1);
    rreq.set_nmessages(1);
    rreq.set_start_id(message_id);
    grpc::ClientContext rcontext;
    grpc::Status rstatus = client->ReadPartition(&rcontext, rreq, &rres);
    ASSERT_EQ(LedgerdStatus::OK, rres.ledger_response().status());
    ASSERT_EQ(true, rstatus.ok());
    ASSERT_EQ(true, rres.has_messages());

    const LedgerdMessageSet& messages = rres.messages();
    ASSERT_EQ(1, messages.messages_size());
    EXPECT_EQ(1, messages.partition_num());
    const LedgerdMessage& message = messages.messages(0);
    EXPECT_EQ(message_id, message.id());
    EXPECT_EQ("hello", message.data());

    server->Shutdown();
}

TEST(GrpcInterface, GetTopic) {
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
    treq.set_name("grpc_interface_open_topic");
    treq.add_partition_ids(0);
    treq.add_partition_ids(1);
    treq.add_partition_ids(2);

    grpc::ClientContext ocontext;
    grpc::Status ostatus = client->OpenTopic(&ocontext, treq, &tres);
    ASSERT_EQ(true, ostatus.ok());
    ASSERT_EQ(LedgerdStatus::OK, tres.status());

    TopicRequest request;
    TopicResponse response;
    request.set_topic_name("grpc_interface_open_topic");
    grpc::ClientContext context;

    grpc::Status status = client->GetTopic(&context, request, &response);
    ASSERT_EQ(true, status.ok());
    EXPECT_EQ(true, response.found());
    EXPECT_EQ("grpc_interface_open_topic", response.topic().name());
    EXPECT_EQ(3, response.topic().npartitions());

    grpc::ClientContext c2;
    response.Clear();
    request.set_topic_name("not_found");
    status = client->GetTopic(&c2, request, &response);
    ASSERT_EQ(true, status.ok());
    EXPECT_EQ(false, response.found());

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
    treq.add_partition_ids(0);
    treq.add_partition_ids(1);

    grpc::ClientContext ocontext;
    grpc::Status ostatus = client->OpenTopic(&ocontext, treq, &tres);
    ASSERT_EQ(true, ostatus.ok());
    ASSERT_EQ(LedgerdStatus::OK, tres.status());

    WritePartitionRequest wreq;
    WriteResponse wres;
    wreq.set_topic_name("grpc_interface_topic");
    wreq.set_partition_num(1);
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
    sreq.set_partition_num(1);
    sreq.set_start_id(message_id);
    sreq.set_read_chunk_size(64);

    grpc::ClientContext rcontext;
    std::unique_ptr<grpc::ClientReader<LedgerdMessageSet>> reader(client->StreamPartition(&rcontext, sreq));

    ASSERT_EQ(true, reader->Read(&messages));
    rcontext.TryCancel();
    grpc::Status sstatus = reader->Finish();

    ASSERT_EQ(grpc::StatusCode::CANCELLED, sstatus.error_code());
    ASSERT_EQ(1, messages.messages_size());
    EXPECT_EQ(1, messages.partition_num());

    const LedgerdMessage& message = messages.messages(0);
    EXPECT_EQ(message_id, message.id());
    EXPECT_EQ("hello", message.data());

    server->Shutdown();
}

TEST(GrpcInterface, Stream) {
    const std::string topic_name = "grpc_interface_stream_topic";
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
    treq.set_name(topic_name);
    treq.add_partition_ids(0);
    treq.add_partition_ids(1);

    grpc::ClientContext ocontext;
    grpc::Status ostatus = client->OpenTopic(&ocontext, treq, &tres);
    ASSERT_EQ(true, ostatus.ok());
    ASSERT_EQ(LedgerdStatus::OK, tres.status());

    StreamRequest sreq;
    PositionSettings* position_settings = sreq.mutable_position_settings();
    position_settings->set_behavior(PositionBehavior::FORGET);
    sreq.set_topic_name(topic_name);
    sreq.add_partition_ids(0);
    sreq.add_partition_ids(1);
    sreq.set_read_chunk_size(64);

    grpc::ClientContext rcontext;
    std::unique_ptr<grpc::ClientReader<LedgerdMessageSet>> reader(client->Stream(&rcontext, sreq));

    sleep(1);

    LedgerdMessageSet m1;
    LedgerdMessageSet m2;

    WritePartitionRequest wreq;
    WriteResponse wres;
    wreq.set_topic_name(topic_name);
    wreq.set_partition_num(0);
    wreq.set_data("hello");
    grpc::ClientContext wcontext;
    grpc::Status wstatus = client->WritePartition(&wcontext, wreq, &wres);
    ASSERT_EQ(LedgerdStatus::OK, wres.ledger_response().status());
    ASSERT_EQ(true, wstatus.ok());

    ASSERT_EQ(true, reader->Read(&m1));

    WritePartitionRequest wreq2;
    WriteResponse wres2;
    wreq2.set_topic_name(topic_name);
    wreq2.set_partition_num(1);
    wreq2.set_data("there");
    grpc::ClientContext wcontext2;
    grpc::Status wstatus2 = client->WritePartition(&wcontext2, wreq2, &wres2);
    ASSERT_EQ(LedgerdStatus::OK, wres.ledger_response().status());
    ASSERT_EQ(true, wstatus2.ok());

    ASSERT_EQ(true, reader->Read(&m2));
    rcontext.TryCancel();
    grpc::Status sstatus = reader->Finish();

    ASSERT_EQ(grpc::StatusCode::CANCELLED, sstatus.error_code());
    ASSERT_EQ(1, m1.messages_size());
    ASSERT_EQ(1, m2.messages_size());
    EXPECT_EQ(0, m1.partition_num());
    EXPECT_EQ(1, m2.partition_num());

    const LedgerdMessage& mp1 = m1.messages(0);
    EXPECT_EQ("hello", mp1.data());

    const LedgerdMessage& mp2 = m2.messages(0);
    EXPECT_EQ("there", mp2.data());

    server->Shutdown();
}

}
