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
        { 0, 0, 0, 0}
    };
    int ch;
    std::unique_ptr<LedgerdServiceConfig> config = std::unique_ptr<LedgerdServiceConfig>(new LedgerdServiceConfig());

    while((ch = getopt_long(argc, argv, "r:", longopts, NULL)) != -1) {
        switch(ch) {
            case 'r':
                config->set_root_directory(std::string(optarg, strlen(optarg)));
                break;
            default:
                break;
        }
    }
    
    return config;
}

}
