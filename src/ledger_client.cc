#include <iostream>

#include <grpc++/channel_arguments.h>
#include <grpc++/credentials.h>
#include <grpc++/create_channel.h>

#include "client.h"

using namespace ledgerd;

int main(int argc, char **argv) {
    LedgerdClient client(
        grpc::CreateChannel("localhost:50051", grpc::InsecureCredentials(),
                            grpc::ChannelArguments()));
    std::string reply = client.Ping();
    std::cout << reply << std::endl;

    return 0;
}
