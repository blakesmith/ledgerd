#include <memory>

#include "proto/ledgerd.grpc.pb.h"

#include <grpc/grpc.h>
#include <grpc++/channel_interface.h>
#include <grpc++/client_context.h>

namespace ledgerd {
class LedgerdClient {
    std::unique_ptr<Ledgerd::Stub> stub_;
public:
    LedgerdClient(std::shared_ptr<grpc::ChannelInterface> channel);
    std::string Ping();
};
}
