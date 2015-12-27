#include "cluster_manager.h"

#include <vector>

namespace ledgerd {

ClusterManager::ClusterManager(uint32_t this_node_id,
                               LedgerdService& ledger_service)
    : this_node_id_(this_node_id),
      ledger_service_(ledger_service),
      cluster_log_(ledger_service),
      paxos_group_(this_node_id, cluster_log_) { }

grpc::Status ClusterManager::ProcessPaxos(grpc::ServerContext* context,
                                          const PaxosMessage* request,
                                          PaxosMessage* response) {
    return grpc::Status::OK;
}

const paxos::Message<ClusterEvent> ClusterManager::map_internal(const PaxosMessage* in) const {
    paxos::MessageType type;
    switch(in->message_type()) {
        case PaxosMessageType::PREPARE:
            type = paxos::MessageType::PREPARE;
            break;
        case PaxosMessageType::PROMISE:
            type = paxos::MessageType::PROMISE;
            break;
        case PaxosMessageType::REJECT:
            type = paxos::MessageType::REJECT;
            break;
        case PaxosMessageType::ACCEPT:
            type = paxos::MessageType::ACCEPT;
            break;
        case PaxosMessageType::ACCEPTED:
            type = paxos::MessageType::ACCEPTED;
            break;
        case PaxosMessageType::DECIDED:
            type = paxos::MessageType::DECIDED;
            break;
        default:
            throw std::invalid_argument("Invalid message type");
            break;
    };
    paxos::ProposalId proposal_id(in->proposal_id().node_id(),
                                  in->proposal_id().prop_n());
    return paxos::Message<ClusterEvent>(
        type,
        in->sequence(),
        proposal_id,
        in->source_node_id(),
        std::vector<uint32_t> { this_node_id_ },
        &in->event());
}

void ClusterManager::map_external(const paxos::Message<ClusterEvent>* in, PaxosMessage* out) const {
}

}
