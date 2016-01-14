#ifndef LEDGERD_GRPC_INTERFACE_H_
#define LEDGERD_GRPC_INTERFACE_H_

#include "proto/ledgerd.grpc.pb.h"
#include "cluster_manager.h"
#include "ledgerd_service.h"

#include <grpc/grpc.h>
#include <grpc++/server_context.h>

namespace ledgerd {
class GrpcInterface final : public Ledgerd::Service {
    LedgerdService& ledgerd_service_;
    ClusterManager& cluster_manager_;
public:
    GrpcInterface(LedgerdService& ledgerd_service,
                  ClusterManager& cluster_manager);
    grpc::Status Ping(grpc::ServerContext *context, const PingRequest *req,
                      PingResponse *resp) override;

    grpc::Status OpenTopic(grpc::ServerContext *context, const OpenTopicRequest *req,
                           LedgerdResponse *resp) override;

    grpc::Status GetTopic(grpc::ServerContext *context, const TopicRequest *request,
                          TopicResponse *response) override;

    grpc::Status WritePartition(grpc::ServerContext *context, const WritePartitionRequest *req,
                                WriteResponse *resp) override;

    LedgerdStatus translate_status(ledger_status rc);

    grpc::Status ReadPartition(grpc::ServerContext *context, const ReadPartitionRequest *req,
                               ReadResponse *resp) override;

    grpc::Status StreamPartition(grpc::ServerContext *context, const StreamPartitionRequest* request, grpc::ServerWriter<LedgerdMessageSet>* writer) override;

    grpc::Status Stream(grpc::ServerContext *context, const StreamRequest* request, grpc::ServerWriter<LedgerdMessageSet>* writer) override;
};
}

#endif
