#ifndef LEDGERD_SERVICE_H_
#define LEDGERD_SERVICE_H_

#include "ledger.h"

#include "ledgerd_service_config.h"

namespace ledgerd {
class LedgerdService final {
    LedgerdServiceConfig config_;
    ledger_ctx ctx;
public:
    LedgerdService(const LedgerdServiceConfig& config);
    ~LedgerdService();
};
}

#endif
