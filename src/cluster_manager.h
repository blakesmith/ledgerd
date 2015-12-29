#ifndef LEDGER_CLUSTER_MANAGER_H_
#define LEDGER_CLUSTER_MANAGER_H_

#include "proto/ledgerd.grpc.pb.h"

#include "cluster_log.h"
#include "paxos/group.h"

#include <grpc/grpc.h>
#include <grpc++/grpc++.h>
#include <grpc++/server_context.h>

namespace ledgerd {

template <typename C, typename T>
class AsyncClientRPC {
    C* stub_;
    std::unique_ptr<T> reply_;
    std::unique_ptr<grpc::ClientContext> client_context_;
    std::unique_ptr<grpc::ClientAsyncResponseReader<PaxosMessage>> reader_;
public:
    AsyncClientRPC()
        : stub_(nullptr),
          reader_(nullptr) { }

    C* stub() {
        return stub_;
    }

    void set_stub(C* stub) {
        this->stub_ = stub;
    }

    T* reply() {
        return reply_.get();
    }

    grpc::ClientContext* client_context() {
        return client_context_.get();
    }

    grpc::ClientAsyncResponseReader<T>* reader() {
        return reader_.get();
    }
};

class ClusterManager : public Clustering::Service {
    uint32_t this_node_id_;
    LedgerdService& ledger_service_;
    ClusterLog cluster_log_;
    paxos::Group<ClusterEvent> paxos_group_;

    void node_connection(uint32_t node_id, Clustering::Stub* stub);

    const paxos::Message<ClusterEvent> map_internal(const PaxosMessage* in) const;

    void send_messages(uint32_t source_node_id,
                       const std::vector<paxos::Message<ClusterEvent>>& messages,
                       PaxosMessage* response);

    void map_external(const paxos::Message<ClusterEvent>* in,
                      PaxosMessage* out) const;
public:
    ClusterManager(uint32_t this_node_id,
                   LedgerdService& ledger_service);

    void Start();

    grpc::Status ProcessPaxos(grpc::ServerContext* context,
                              const PaxosMessage* request,
                              PaxosMessage* response);
};

}

#endif
