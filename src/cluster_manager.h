#ifndef LEDGER_CLUSTER_MANAGER_H_
#define LEDGER_CLUSTER_MANAGER_H_

#include <atomic>
#include <map>
#include <thread>

#include "proto/ledgerd.grpc.pb.h"

#include "cluster_log.h"
#include "node_info.h"
#include "paxos/group.h"

#include <grpc/grpc.h>
#include <grpc++/grpc++.h>
#include <grpc++/server.h>
#include <grpc++/server_context.h>

namespace ledgerd {


template <typename C, typename T>
class AsyncClientRPC {
    uint32_t id_;
    C* stub_;
    T reply_;
    grpc::ClientContext client_context_;
    grpc::Status status_;
    std::unique_ptr<grpc::ClientAsyncResponseReader<PaxosMessage>> reader_;
public:
    AsyncClientRPC(uint32_t id)
        : id_(id),
          stub_(nullptr),
          reader_(nullptr) { }

    ~AsyncClientRPC() = default;

    uint32_t id() const {
        return id_;
    }

    C* stub() const {
        return stub_;
    }

    void set_reader(std::unique_ptr<grpc::ClientAsyncResponseReader<T>> reader) {
        this->reader_ = std::move(reader);
    }

    T* reply() {
        return &reply_;
    }
    
    grpc::Status* status() {
        return &status_;
    }

    grpc::ClientContext* client_context() {
        return &client_context_;
    }

    grpc::ClientAsyncResponseReader<T>* reader() const {
        return reader_.get();
    }
};

class ClusterManager : public Clustering::Service {
    uint32_t this_node_id_;
    uint32_t next_rpc_id_;
    LedgerdService& ledger_service_;
    std::unique_ptr<grpc::Server> cluster_server_;
    std::string grpc_cluster_address_;
    ClusterLog cluster_log_;
    grpc::CompletionQueue cq_;
    std::thread async_thread_;
    std::atomic<bool> async_thread_run_;
    paxos::Group<ClusterEvent> paxos_group_;
    std::map<uint32_t, NodeInfo> node_info_;
    std::map<uint32_t, std::unique_ptr<Clustering::Stub>> connections_;
    std::map<uint32_t, std::unique_ptr<AsyncClientRPC<Clustering::Stub, PaxosMessage>>> in_flight_rpcs_;
    std::mutex rpc_mutex_;

    void start_async_thread();

    void stop_async_thread();

    void start_grpc_interface();

    void async_loop();

    void node_connection(uint32_t node_id, Clustering::Stub** stub);

    void log_paxos_message(const std::string& location,
                           const PaxosMessage* message) const;

    const paxos::Message<ClusterEvent> map_internal(const PaxosMessage* in) const;

    void send_messages(uint32_t source_node_id,
                       const std::vector<paxos::Message<ClusterEvent>>& messages,
                       PaxosMessage* response);

    void map_external(const paxos::Message<ClusterEvent>* in,
                      PaxosMessage* out) const;

    uint64_t send(std::unique_ptr<ClusterEvent> message);
public:
    ClusterManager(uint32_t this_node_id,
                   LedgerdService& ledger_service,
                   const std::string& grpc_cluster_address,
                   std::map<uint32_t, NodeInfo> node_info);

    void Start();

    void Stop();

    uint64_t RegisterTopic(const std::string& topic_name,
                           const std::vector<unsigned int>& partition_ids);

    void WaitForSequence(uint64_t sequence);

    std::unique_ptr<ClusterEvent> GetEvent(uint64_t sequence);

    grpc::Status ProcessPaxos(grpc::ServerContext* context,
                              const PaxosMessage* request,
                              PaxosMessage* response);
};

}

#endif
