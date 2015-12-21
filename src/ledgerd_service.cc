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

ledger_status LedgerdService::OpenTopic(const std::string& name,
                                        const std::vector<unsigned int>& partition_ids,
                                        ledger_topic_options *options) {
    return ledger_open_topic(&ctx,
                             name.c_str(),
                             partition_ids.data(),
                             partition_ids.size(),
                             options);
}

ledger_topic *LedgerdService::GetTopic(const std::string& name) {
    return ledger_lookup_topic(&ctx, name.c_str());
}

ledger_status LedgerdService::WritePartition(const std::string& topic_name,
                                             uint32_t partition_number,
                                             const std::string& data,
                                             ledger_write_status *status) {
    return ledger_write_partition(&ctx, topic_name.c_str(), partition_number, const_cast<char*>(data.data()), data.size(), status);
}

ledger_status LedgerdService::ReadPartition(const std::string& topic_name,
                                            uint32_t partition_number,
                                            uint64_t start_id,
                                            uint32_t nmessages,
                                            ledger_message_set *messages) {
    return ledger_read_partition(&ctx, topic_name.c_str(), partition_number, start_id, nmessages, messages);
}

ledger_status LedgerdService::LatestMessageId(const std::string& topic_name,
                                              uint32_t partition_number,
                                              uint64_t *id) {
    return ledger_latest_message_id(&ctx, topic_name.c_str(), partition_number, id);
}

ledger_status LedgerdService::StartConsumer(Consumer* consumer,
                                            const std::string& topic_name,
                                            uint32_t partition_number,
                                            uint64_t start_id) {
    ledger_status rc;
    rc = consumer->Attach(&ctx, topic_name, partition_number);
    if(rc != LEDGER_OK) {
        return rc;
    }
    return consumer->Start(start_id);
}

ledger_status LedgerdService::StartConsumerGroup(ConsumerGroup* group,
                                                 const std::string& topic_name,
                                                 std::vector<unsigned int> partition_ids) {
    ledger_status rc;
    rc = group->Attach(&ctx, topic_name, partition_ids);
    if(rc != LEDGER_OK) {
        return rc;
    }
    return group->Start();
}
}
