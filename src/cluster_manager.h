#ifndef LEDGER_CLUSTER_MANAGER_H_
#define LEDGER_CLUSTER_MANAGER_H_

#include "proto/ledgerd.grpc.pb.h"

#include "cluster_log.h"
#include "paxos/group.h"

#include <grpc/grpc.h>
#include <grpc++/server_context.h>

namespace ledgerd {

class ClusterManager : public Clustering::Service {
    uint32_t this_node_id_;
    LedgerdService& ledger_service_;
    ClusterLog cluster_log_;
    paxos::Group<ClusterEvent> paxos_group_;

    const paxos::Message<ClusterEvent> map_internal(const PaxosMessage* in) const;

    void map_external(const paxos::Message<ClusterEvent>* in,
                      PaxosMessage* out) const;
public:
    ClusterManager(uint32_t this_node_id,
                   LedgerdService& ledger_service);

    grpc::Status ProcessPaxos(grpc::ServerContext* context,
                              const PaxosMessage* request,
                              PaxosMessage* response);
};

}

#endif
