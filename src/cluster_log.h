#ifndef LEDGERD_CLUSTER_LOG_H_
#define LEDRERD_CLUSTER_LOG_H_

#include <memory>

#include "ledgerd_service.h"

#include "paxos/persistent_log.h"
#include "proto/admin.pb.h"

namespace ledgerd {

class ClusterLog : public paxos::PersistentLog<ClusterEvent> {
    static const std::string TOPIC_NAME;
    LedgerdService& ledger_service_;
public:
    ClusterLog(LedgerdService& ledger_service);
    ~ClusterLog();

    virtual paxos::LogStatus Write(uint64_t sequence, const ClusterEvent* event);
    virtual std::unique_ptr<ClusterEvent> Get(uint64_t sequence);
    virtual uint64_t HighestSequence();
};
}

#endif
