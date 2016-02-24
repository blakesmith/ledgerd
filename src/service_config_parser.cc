#include <getopt.h>
#include <cstring>
#include <iostream>

#include "service_config_parser.h"

namespace ledgerd {

ServiceConfigParser::ServiceConfigParser() {
    optind = 1;
}

void ServiceConfigParser::print_help() {
    std::cout << "Usage: ledgerd [ options ...]" << std::endl;
    std::cout << "    -H --help                    Print this message and exit." << std::endl;
    std::cout << "    -r --root                    Root data directory. Default: /var/lib/ledgerd." << std::endl;
    std::cout << "    -g --grpc-address            GRPC interface address. Default: 0.0.0.0:50051." << std::endl;
    std::cout << "    -c --cluster-address         Cluster gossip interface address. Default: 0.0.0.0:50052." << std::endl;
    std::cout << "    -i --cluster-node-id         Cluster node id." << std::endl;
    std::cout << "    -p --cluster-partition-count Default new topic partition count. Default: 1" << std::endl;
}

std::unique_ptr<LedgerdServiceConfig> ServiceConfigParser::MakeServiceConfig(int argc, char **argv) {
    static struct option longopts[] = {
        { "help", no_argument, 0, 'H' },
        { "root", required_argument, 0, 'r' },
        { "grpc-address", required_argument, 0, 'g' },
        { "cluster-address", required_argument, 0, 'c' },
        { "cluster-node-id", required_argument, 0, 'i' },
        { "default-partition-count", required_argument, 0, 'p' },
        { 0, 0, 0, 0}
    };
    int ch;
    std::unique_ptr<LedgerdServiceConfig> config = std::unique_ptr<LedgerdServiceConfig>(new LedgerdServiceConfig());

    while((ch = getopt_long(argc, argv, "h:r:g:c:i:p:", longopts, NULL)) != -1) {
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
            case 'H':
            default:
                print_help();
                return nullptr;
                break;
        }
    }
    
    return config;
}

}
