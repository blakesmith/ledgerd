#include "client.h"

namespace ledgerd {
LedgerdClient::LedgerdClient(std::shared_ptr<grpc::ChannelInterface> channel)
    : stub_(Ledgerd::NewStub(channel)) {}

std::string LedgerdClient::Ping() {
    PingRequest request;
    PingResponse response;
    request.set_ping("ping");
    grpc::ClientContext context;

    grpc::Status status = stub_->Ping(&context, request, &response);
    if(status.ok()) {
        return response.pong();
    } else {
        return "Rpc failed";
    }
}
}
