#include "cluster_manager.h"

namespace ledgerd {

ClusterManager::ClusterManager(uint32_t this_node_id,
                               LedgerdService& ledger_service)
    : ledger_service_(ledger_service),
      cluster_log_(ledger_service),
      paxos_group_(this_node_id, cluster_log_) { }
}
