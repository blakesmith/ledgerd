#ifndef LEDGERD_SERVICE_CONFIG_PARSER_H_
#define LEDGERD_SERVICE_CONFIG_PARSER_H_

#include <memory>

#include "ledgerd_service_config.h"

namespace ledgerd {
class ServiceConfigParser final {
public:
    ServiceConfigParser();
    std::unique_ptr<LedgerdServiceConfig> MakeServiceConfig(int argc, char **argv);
};
}

#endif
