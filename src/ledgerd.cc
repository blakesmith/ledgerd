#include <iostream>
#include <memory>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_credentials.h>

#include "ledgerd_service.h"
#include "ledgerd_service_config.h"
#include "grpc_interface.h"

using namespace ledgerd;

int main(int argc, char **argv) {
    // TODO: Populate config
    LedgerdServiceConfig config;
    LedgerdService ledgerd_service(config);
    GrpcInterface grpc_interface(ledgerd_service);;
    grpc::ServerBuilder builder;
    std::string server_address("0.0.0.0:50051");
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&grpc_interface);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();

    return 0;
}
