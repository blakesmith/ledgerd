#include <memory>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/security/server_credentials.h>

#include "cluster_manager.h"
#include "ledgerd_service.h"
#include "log.h"
#include "service_config_parser.h"
#include "ledgerd_service_config.h"
#include "grpc_interface.h"

using namespace ledgerd;

int main(int argc, char **argv) {
    ServiceConfigParser parser;
    auto config = parser.MakeServiceConfig(argc, argv);
    if(config == nullptr) {
        exit(1);
    }
    LedgerdService ledgerd_service(*config);
    ClusterManager cluster_manager(config->cluster_node_id(),
                                   ledgerd_service,
                                   config->grpc_cluster_address(),
                                   config->node_info());
    cluster_manager.Start();
    GrpcInterface grpc_interface(ledgerd_service, cluster_manager);
    grpc::ServerBuilder builder;
    builder.AddListeningPort(config->get_grpc_address(), grpc::InsecureServerCredentials());
    builder.RegisterService(&grpc_interface);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    LEDGERD_LOG(logINFO) << "Server listening on " << config->get_grpc_address();
    server->Wait();
    cluster_manager.Stop();

    return 0;
}
