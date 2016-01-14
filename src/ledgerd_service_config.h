#ifndef LEDGERD_SERVICE_CONFIG_H_
#define LEDGERD_SERVICE_CONFIG_H_

#include <map>
#include <string>

#include "node_info.h"

namespace ledgerd {

class LedgerdServiceConfig {
    std::string root_directory_;
    std::string grpc_address_;

    uint32_t cluster_node_id_;
    std::string grpc_cluster_address_;
    std::map<uint32_t, NodeInfo> node_info_;
public:
    LedgerdServiceConfig();
    LedgerdServiceConfig(const std::string& root_directory,
                         const std::string& grpc_address);
    void set_root_directory(const std::string& root_directory);
    const std::string& get_root_directory() const;

    void set_grpc_address(const std::string& grpc_address);
    const std::string& get_grpc_address() const;

    void set_grpc_cluster_address(const std::string& grpc_cluster_address);
    const std::string& grpc_cluster_address() const;

    void set_cluster_node_id(uint32_t node_id);
    const uint32_t cluster_node_id() const;

    void add_node(uint32_t node_id, const std::string& host_and_port);
    const std::map<uint32_t, NodeInfo>& node_info() const;
};
}

#endif
