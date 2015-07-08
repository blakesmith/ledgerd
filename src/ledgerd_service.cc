#include <stdexcept>

#include "ledgerd_service.h"

namespace ledgerd {
LedgerdService::LedgerdService(const LedgerdServiceConfig& config)
    : config_(config) {
    ledger_status rc;
    const std::string& root_directory = config.get_root_directory();
    rc = ledger_open_context(&ctx, root_directory.c_str());
    if(rc != LEDGER_OK) {
        throw std::runtime_error("Failed to open ledger context");
    }
}

LedgerdService::~LedgerdService() {
    ledger_close_context(&ctx);
}

ledger_status LedgerdService::OpenTopic(const std::string& name, uint32_t partition_count, ledger_topic_options *options) {
    return ledger_open_topic(&ctx, name.c_str(), partition_count, options);
}

ledger_status LedgerdService::WritePartition(const std::string& topic_name, uint32_t partition_number, const std::string& data, ledger_write_status *status) {
    return ledger_write_partition(&ctx, topic_name.c_str(), partition_number, const_cast<char*>(data.data()), data.size(), status);
}
}
