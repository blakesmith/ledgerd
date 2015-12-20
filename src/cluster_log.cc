#include "cluster_log.h"

namespace ledgerd {

ClusterLog::ClusterLog(LedgerdService& ledger_service)
    : ledger_service_(ledger_service) { }

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
