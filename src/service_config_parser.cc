#include <getopt.h>
#include <cstring>

#include "service_config_parser.h"

namespace ledgerd {

ServiceConfigParser::ServiceConfigParser() {
    optind = 1;
}

std::unique_ptr<LedgerdServiceConfig> ServiceConfigParser::MakeServiceConfig(int argc, char **argv) {
    static struct option longopts[] = {
        { "root", required_argument, 0, 'r' },
        { "grpc-address", required_argument, 0, 'g' },
        { "cluster-address", required_argument, 0, 'c' },
        { "cluster-node-id", required_argument, 0, 'i' },
        { "default-partition-count", required_argument, 0, 'p' },
        { 0, 0, 0, 0}
    };
    int ch;
    std::unique_ptr<LedgerdServiceConfig> config = std::unique_ptr<LedgerdServiceConfig>(new LedgerdServiceConfig());

    while((ch = getopt_long(argc, argv, "r:g:c:i:n:", longopts, NULL)) != -1) {
        switch(ch) {
            case 'r':
                config->set_root_directory(std::string(optarg, strlen(optarg)));
                break;
            case 'g':
                config->set_grpc_address(std::string(optarg, strlen(optarg)));
                break;
            case 'c':
                config->set_grpc_cluster_address(std::string(optarg, strlen(optarg)));
                break;
            case 'i':
                config->set_cluster_node_id(atoi(optarg));
                break;
            case 'p':
                config->set_default_partition_count(atoi(optarg));
                break;
            default:
                break;
        }
    }
    
    return config;
}

}
