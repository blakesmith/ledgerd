#ifndef LEDGERD_CLUSTER_LOG_H_
#define LEDRERD_CLUSTER_LOG_H_

#include "paxos/persistent_log.h"

namespace ledgerd {

struct ClusterEvent {};

class ClusterLog : public paxos::PersistentLog<ClusterEvent> {
public:
    paxos::LogStatus Write(uint64_t sequence, const ClusterEvent* event);
    std::unique_ptr<ClusterEvent> Get(uint64_t sequence);
    uint64_t HighestSequence();
};
}

#endif
