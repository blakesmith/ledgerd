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

    ledger_status OpenTopic(const std::string& name, uint32_t partition_count, ledger_topic_options *options);
    ledger_status WritePartition(const std::string& topic_name, uint32_t partition_number, const std::string& data, ledger_write_status *status);
};
}

#endif
