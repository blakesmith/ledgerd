#ifndef LEDGERD_GRPC_INTERFACE_H_
#define LEDGERD_GRPC_INTERFACE_H_

#include "proto/ledgerd.grpc.pb.h"
#include "ledgerd_service.h"

#include <grpc/grpc.h>
#include <grpc++/server_context.h>

namespace ledgerd {
class GrpcInterface final : public Ledgerd::Service {
    LedgerdService& ledgerd_service_;
public:
    GrpcInterface(LedgerdService& ledgerd_service);
    grpc::Status Ping(grpc::ServerContext *context, const PingRequest *req,
                      PingResponse *resp) override;

    grpc::Status OpenTopic(grpc::ServerContext *context, const OpenTopicRequest *req,
                           LedgerdResponse *resp) override;

    LedgerdStatus translate_status(ledger_status rc);
};
}

#endif
