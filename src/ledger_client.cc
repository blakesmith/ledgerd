#include <iostream>

#include <grpc++/channel_arguments.h>
#include <grpc++/credentials.h>
#include <grpc++/create_channel.h>

#include "client.h"

using namespace ledgerd;

static void do_ping(LedgerdClient& client) {
    std::cout << "Sending ping" << std::endl;
    std::string reply = client.Ping();
    std::cout << reply << std::endl;
}

static void do_open_topic(LedgerdClient& client, const std::string& topic_name) {
    OpenTopicRequest req;
    LedgerdResponse res;
    std::cout << "Sending open topic" << std::endl;
    grpc::Status status = client.OpenTopic(req, &res);
    if(status.ok()) {
        if(res.status() == LedgerdStatus::LEDGER_OK) {
            std::cout << "Successfully opened topic" << std::endl;
        }
    }
}

int main(int argc, char **argv) {
    LedgerdClient client(
        grpc::CreateChannel("localhost:50051", grpc::InsecureCredentials(),
                            grpc::ChannelArguments()));
    do_ping(client);
    do_open_topic(client, "some_topic");
    return 0;
}
