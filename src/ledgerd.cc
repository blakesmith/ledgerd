#include <iostream>
#include <memory>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/security/server_credentials.h>

#include "cluster_manager.h"
#include "ledgerd_service.h"
#include "ledgerd_service_config.h"
#include "grpc_interface.h"

using namespace ledgerd;

int main(int argc, char **argv) {
    // TODO: Populate config correctly
    LedgerdServiceConfig config;
    config.set_grpc_address("0.0.0.0:64399");
    config.set_root_directory("/tmp/ledgerd");
    config.set_grpc_cluster_address("0.0.0.0:64398");
    config.set_cluster_node_id(0);
    config.add_node(0, "0.0.0.0:64338");

    LedgerdService ledgerd_service(config);
    ClusterManager cluster_manager(config.cluster_node_id(),
                                   ledgerd_service,
                                   config.grpc_cluster_address(),
                                   config.node_info());
    cluster_manager.Start();
    GrpcInterface grpc_interface(ledgerd_service, cluster_manager);
    grpc::ServerBuilder builder;
    builder.AddListeningPort(config.get_grpc_address(), grpc::InsecureServerCredentials());
    builder.RegisterService(&grpc_interface);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << config.get_grpc_address() << std::endl;
    server->Wait();
    cluster_manager.Stop();

    return 0;
}
