#include <memory>
#include <string>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_credentials.h>
#include <grpc++/status.h>

#include "server.h"


namespace ledgerd {
grpc::Status LedgerdServiceImpl::Ping(grpc::ServerContext *context, const PingRequest *req,
                                      PingResponse *resp) {
    resp->set_pong("pong");
    return grpc::Status::OK;
}
}
