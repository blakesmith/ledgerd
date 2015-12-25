#ifndef LEDGER_CLUSTER_MANAGER_H_
#define LEDGER_CLUSTER_MANAGER_H_

#include "cluster_log.h"
#include "paxos/group.h"

namespace ledgerd {

class ClusterManager {
    LedgerdService& ledger_service_;
    ClusterLog cluster_log_;
    paxos::Group<ClusterEvent> paxos_group_;
public:
    ClusterManager(uint32_t this_node_id,
                   LedgerdService& ledger_service);
};

}

#endif
