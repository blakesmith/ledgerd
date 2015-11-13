#ifndef LEDGERD_SERVICE_H_
#define LEDGERD_SERVICE_H_

#include "ledger.h"

#include "ledgerd_consumer.h"
#include "ledgerd_service_config.h"

namespace ledgerd {
class LedgerdService final {
    LedgerdServiceConfig config_;
    ledger_ctx ctx;
public:
    LedgerdService(const LedgerdServiceConfig& config);
    ~LedgerdService();

    ledger_status OpenTopic(const std::string& name, uint32_t partition_count, ledger_topic_options *options);

    ledger_topic *GetTopic(const std::string& name);

    ledger_status WritePartition(const std::string& topic_name, uint32_t partition_number, const std::string& data, ledger_write_status *status);

    ledger_status ReadPartition(const std::string& topic_name, uint32_t partition_number, uint64_t start_id, uint32_t nmessages, ledger_message_set *messages);

    ledger_status StartConsumer(Consumer* consumer, const std::string& topic_name, uint32_t partition_number, uint64_t start_id);
};
}

#endif
