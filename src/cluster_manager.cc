#include "cluster_manager.h"

#include <vector>

namespace ledgerd {

NodeInfo::NodeInfo(const std::string& host_and_port)
    : host_and_port_(host_and_port) { }

const std::string& NodeInfo::host_and_port() const {
    return host_and_port_;
}

ClusterManager::ClusterManager(uint32_t this_node_id,
                               LedgerdService& ledger_service,
                               std::map<uint32_t, NodeInfo> node_info)
    : this_node_id_(this_node_id),
      ledger_service_(ledger_service),
      node_info_(node_info),
      cluster_log_(ledger_service),
      paxos_group_(this_node_id, cluster_log_) { }

void ClusterManager::Start() {
    paxos_group_.Start();
}

void ClusterManager::node_connection(uint32_t node_id, Clustering::Stub** stub) {
    auto search = connections_.find(node_id);
    if(search != connections_.end()) {
        *stub = search->second.get();
        return;
    }
    auto connection_info = node_info_.find(node_id);
    if(connection_info != node_info_.end()) {
        std::unique_ptr<Clustering::Stub> new_stub = Clustering::NewStub(
            grpc::CreateChannel(connection_info->second.host_and_port(),
                                grpc::InsecureChannelCredentials()));
        *stub = new_stub.get();
        connections_[node_id] = std::move(new_stub);
    }

    // TODO: Log the connection error
    *stub = nullptr;
}

void ClusterManager::send_messages(uint32_t source_node_id,
                                   const std::vector<paxos::Message<ClusterEvent>>& messages,
                                   PaxosMessage* response) {
    for(auto& m : messages) {
        std::vector<uint32_t> rpc_node_ids;
        for(uint32_t node_id : m.target_node_ids()) {
            if(node_id == source_node_id) {
                map_external(&m, response);
            } else {
                rpc_node_ids.push_back(node_id);
            }
        }

        if(rpc_node_ids.size() <= 0) {
            return;
        }

        PaxosMessage request;
        map_external(&m, &request);
        grpc::CompletionQueue cq;
        std::unique_ptr<AsyncClientRPC<
                Clustering::Stub, PaxosMessage>[]> rpcs(
                    new AsyncClientRPC<Clustering::Stub, PaxosMessage>[rpc_node_ids.size()]);
        for(int i = 0; i < rpc_node_ids.size(); ++i) {
            uint32_t node_id = rpc_node_ids[i];
            auto& rpc = rpcs[i];
            Clustering::Stub *stub = rpc.stub();
            node_connection(node_id, &stub);
            if(stub != nullptr) {
                std::unique_ptr<grpc::ClientAsyncResponseReader<PaxosMessage>> reader(
                    stub->AsyncProcessPaxos(rpc.client_context(), request, &cq));
                reader->Finish(rpc.reply(),
                               rpc.status(),
                               nullptr);
                rpc.set_reader(std::move(reader));
            }
        }
        // TODO, cq.Next
    }
}

grpc::Status ClusterManager::ProcessPaxos(grpc::ServerContext* context,
                                          const PaxosMessage* request,
                                          PaxosMessage* response) {
    const std::vector<paxos::Message<ClusterEvent>> internal_messages {
        map_internal(request) };
    std::vector<paxos::Message<ClusterEvent>> responses = paxos_group_.Receive(request->sequence(),
                                                                               internal_messages);
    send_messages(request->source_node_id(),
                  internal_messages,
                  response);
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
    PaxosMessageType type;
    switch(in->message_type()) {
        case paxos::MessageType::PREPARE:
            type = PaxosMessageType::PREPARE;
            break;
        case paxos::MessageType::PROMISE:
            type = PaxosMessageType::PROMISE;
            break;
        case paxos::MessageType::REJECT:
            type = PaxosMessageType::REJECT;
            break;
        case paxos::MessageType::ACCEPT:
            type = PaxosMessageType::ACCEPT;
            break;
        case paxos::MessageType::ACCEPTED:
            type = PaxosMessageType::ACCEPTED;
            break;
        case paxos::MessageType::DECIDED:
            type = PaxosMessageType::DECIDED;
            break;
        default:
            throw std::invalid_argument("Invalid message type");
            break;
    };
    PaxosProposalId* proposal_id = out->mutable_proposal_id();
    proposal_id->set_node_id(in->proposal_id().node_id());
    proposal_id->set_prop_n(in->proposal_id().prop_n());
    out->set_message_type(type);
    out->set_sequence(in->sequence());
    out->set_source_node_id(this_node_id_);
    if(in->value()) {
        out->set_allocated_event(const_cast<ClusterEvent*>(in->value()));
    }
}

}
