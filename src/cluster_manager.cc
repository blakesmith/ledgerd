#include "cluster_manager.h"
#include "log.h"

#include <chrono>
#include <vector>

#include <grpc++/server_builder.h>
#include <grpc++/security/server_credentials.h>

namespace ledgerd {

ClusterManager::ClusterManager(uint32_t this_node_id,
                               LedgerdService& ledger_service,
                               const std::string& grpc_cluster_address,
                               std::map<uint32_t, NodeInfo> node_info)
    : this_node_id_(this_node_id),
      next_rpc_id_(0),
      ledger_service_(ledger_service),
      grpc_cluster_address_(grpc_cluster_address),
      node_info_(node_info),
      cluster_log_(ledger_service),
      async_thread_run_(false),
      paxos_group_(this_node_id, cluster_log_) {
    for(auto& kv : node_info) {
        paxos_group_.AddNode(kv.first);
    }
    paxos_group_.AddListener(&cluster_listener_);
}

void ClusterManager::log_paxos_message(const std::string& location,
                                       const PaxosMessage* message) const {
    LEDGERD_LOG(logDEBUG) << location << " paxos message on node: " << this_node_id_
                  << " sequence: " << message->sequence()
                  << " proposal: " << message->proposal_id().prop_n();
}

void ClusterManager::Start() {
    paxos_group_.Start();
    start_async_thread();
    start_grpc_interface();
}

void ClusterManager::Stop() {
    cluster_server_->Shutdown();
    stop_async_thread();
}

void ClusterManager::start_grpc_interface() {
    grpc::ServerBuilder builder;
    builder.AddListeningPort(grpc_cluster_address_, grpc::InsecureServerCredentials());
    builder.RegisterService(this);
    cluster_server_ = builder.BuildAndStart();
}

void ClusterManager::start_async_thread() {
    async_thread_run_.store(true);
    async_thread_ = std::thread(&ClusterManager::async_loop, this);
}

void ClusterManager::stop_async_thread() {
    async_thread_run_.store(false);
    async_thread_.join();
}

void ClusterManager::async_loop() {
    while(async_thread_run_.load()) {
        bool ok;
        void* tag;
        auto current_time = std::chrono::system_clock::now();
        auto deadline = current_time + std::chrono::milliseconds(50);
        auto status = cq_.AsyncNext(&tag, &ok, deadline);
        if (status == grpc::CompletionQueue::NextStatus::GOT_EVENT && ok) {
            AsyncClientRPC<Clustering::Stub, PaxosMessage> *rpc =
                static_cast<AsyncClientRPC<Clustering::Stub, PaxosMessage>*>(tag);
            log_paxos_message("Receiving", rpc->reply());
            const std::vector<paxos::Message<ClusterEvent>> internal_messages {
                map_internal(rpc->reply()) };
            auto requests = paxos_group_.Receive(rpc->reply()->sequence(),
                                                 internal_messages);
            if(requests.size() > 0) {
                send_messages(this_node_id_, requests, nullptr);
            }
            std::lock_guard<std::mutex> lg(rpc_mutex_);
            in_flight_rpcs_.erase(rpc->id());
        } else if(status == grpc::CompletionQueue::NextStatus::SHUTDOWN) {
            return;
        } else {
            continue;
        }

        // Poll for timed out instances
        auto timeouts = paxos_group_.Tick();
        if(timeouts.size() > 0) {
            LEDGERD_LOG(logINFO) << "Received timeout " << timeouts.size() << " messages";
            send_messages(this_node_id_, timeouts, nullptr);
        }
    }
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
        return;
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
                if(response != nullptr) {
                    map_external(&m, response);
                }
            } else if (node_id != this_node_id_) {
                rpc_node_ids.push_back(node_id);
            }
        }

        if(rpc_node_ids.size() <= 0) {
            continue;
        }

        // Make a copy of the request, since the above response lifecycle
        // is owned by GRPC and goes out of scope once that message is
        // sent.
        PaxosMessage request;
        map_external(&m, &request);

        for(int i = 0; i < rpc_node_ids.size(); ++i) {
            ++next_rpc_id_;
            std::unique_ptr<AsyncClientRPC<Clustering::Stub, PaxosMessage>> rpc(
                new AsyncClientRPC<Clustering::Stub, PaxosMessage>(next_rpc_id_));
            uint32_t node_id = rpc_node_ids[i];
            Clustering::Stub *stub = rpc->stub();
            node_connection(node_id, &stub);
            if(stub != nullptr) {
                log_paxos_message("Sending", &request);
                std::unique_ptr<grpc::ClientAsyncResponseReader<PaxosMessage>> reader(
                    stub->AsyncProcessPaxos(rpc->client_context(), request, &cq_));
                reader->Finish(rpc->reply(),
                               rpc->status(),
                               static_cast<void*>(rpc.get()));
                rpc->set_reader(std::move(reader));
                std::lock_guard<std::mutex> lg(rpc_mutex_);
                in_flight_rpcs_[rpc->id()] = std::move(rpc);
            }
        }
    }
}

uint64_t ClusterManager::send(std::unique_ptr<ClusterEvent> event,
                              uint64_t* value_read_id) {
    Node* node = event->mutable_source_node();
    node->set_id(this_node_id_);
    paxos::Instance<ClusterEvent>* new_instance = paxos_group_.CreateInstance();
    auto messages = paxos_group_.Propose(new_instance->sequence(),
                                         std::move(event),
                                         value_read_id);
    send_messages(this_node_id_, messages, nullptr);
    return new_instance->sequence();
}

uint64_t ClusterManager::RegisterTopic(const std::string& topic_name,
                                       const std::vector<unsigned int>& partition_ids) {
    std::unique_ptr<ClusterEvent> event(new ClusterEvent());
    event->set_type(ClusterEventType::REGISTER_TOPIC);
    RegisterTopicEvent* rt = event->mutable_register_topic();
    rt->set_name(topic_name);
    for(unsigned int id : partition_ids) {
        rt->add_partition_ids(id);
    }
    return send(std::move(event), nullptr);
}

void ClusterManager::GetTopics(ClusterTopicList* topic_list) {
    std::unique_ptr<ClusterEvent> event(new ClusterEvent());
    event->set_type(ClusterEventType::LIST_TOPICS);
    uint64_t value_read_id;
    uint64_t sequence = send(std::move(event), &value_read_id);
    std::future<ClusterValue> f = paxos_group_.ReadValue(value_read_id);
    f.wait();
    auto value = f.get();
    assert(value.type == ClusterValueType::TOPIC_LIST);
    *topic_list = value.topic_list;
    paxos_group_.ClearValue(value_read_id);
}

void ClusterManager::WaitForSequence(uint64_t sequence) {
    paxos_group_.WaitForJournaled(sequence);
}

std::unique_ptr<ClusterEvent> ClusterManager::GetEvent(uint64_t sequence) {
    return cluster_log_.Get(sequence);
}

grpc::Status ClusterManager::ProcessPaxos(grpc::ServerContext* context,
                                          const PaxosMessage* request,
                                          PaxosMessage* response) {
    log_paxos_message("Processing", request);
    const std::vector<paxos::Message<ClusterEvent>> internal_messages {
        map_internal(request) };
    std::vector<paxos::Message<ClusterEvent>> responses = paxos_group_.Receive(request->sequence(),
                                                                               internal_messages);
    send_messages(request->source_node_id(),
                  responses,
                  response);
    log_paxos_message("Replying", response);
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
    if(in->has_event()) {
        return paxos::Message<ClusterEvent>(
            type,
            in->sequence(),
            proposal_id,
            in->source_node_id(),
            std::vector<uint32_t> { this_node_id_ },
            &in->event());
    } else {
        return paxos::Message<ClusterEvent>(
            type,
            in->sequence(),
            proposal_id,
            in->source_node_id(),
            std::vector<uint32_t> { this_node_id_ });
    }
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
        out->mutable_event()->CopyFrom(*in->value());
    } else {
        out->clear_event();
    }
}

}
