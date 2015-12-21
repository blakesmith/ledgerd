#include <stdexcept>
#include <sstream>

#include "cluster_log.h"

namespace ledgerd {

const std::string ClusterLog::TOPIC_NAME = "cluster_log";

ClusterLog::ClusterLog(LedgerdService& ledger_service)
    : ledger_service_(ledger_service) {
    ledger_status rc;
    ledger_topic_options options;

    ledger_topic_options_init(&options);
    const std::vector<unsigned int> partition_ids { 0 };
    rc = ledger_service_.OpenTopic(TOPIC_NAME, partition_ids, &options);
    if (rc != LEDGER_OK) {
        std::stringstream ss;
        ss << "Failed to open cluster log topic: " << rc;
        throw std::runtime_error(ss.str());
    }
}

ClusterLog::~ClusterLog() {
    // TODO: Close the topic?
}

paxos::LogStatus ClusterLog::Write(uint64_t sequence, const ClusterEvent* event) {
    return paxos::LogStatus::LOG_OK;
}

std::unique_ptr<ClusterEvent> ClusterLog::Get(uint64_t sequence) {
    return std::unique_ptr<ClusterEvent>(nullptr);
}

uint64_t ClusterLog::HighestSequence() {
    return 0;
}

}
