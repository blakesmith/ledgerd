#ifndef LEDGERD_SERVICE_H_
#define LEDGERD_SERVICE_H_

#include "ledgerd_service_config.h"

namespace ledgerd {
class LedgerdService final {
    LedgerdServiceConfig config_;
public:
    LedgerdService(const LedgerdServiceConfig& config);
};
}

#endif
