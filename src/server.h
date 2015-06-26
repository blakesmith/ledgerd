#include "proto/ledgerd.grpc.pb.h"

#include <grpc/grpc.h>
#include <grpc++/server_context.h>

namespace ledgerd {
class LedgerdServiceImpl final : public Ledgerd::Service {
    grpc::Status Ping(grpc::ServerContext *context, const PingRequest *req,
                      PingResponse *resp) override;
};
}
