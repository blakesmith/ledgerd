#ifndef LEDGERD_CLIENT_H_
#define LEDGERD_CLIENT_H_

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
    grpc::Status OpenTopic(const OpenTopicRequest& req, LedgerdResponse *res);
    grpc::Status WritePartition(const WritePartitionRequest &req, WriteResponse *res);
    grpc::Status ReadPartition(const ReadPartitionRequest &req, ReadResponse *res);
};
}

#endif
