#include "ledgerd_service_config.h"

namespace ledgerd {
LedgerdServiceConfig::LedgerdServiceConfig()
    : root_directory_("/var/lib/ledgerd"),
      grpc_address_("0.0.0.0:50051"),
      grpc_cluster_address_("0.0.0.0:50052") { }

void LedgerdServiceConfig::set_root_directory(const std::string& root_directory) {
    this->root_directory_ = root_directory;
}

const std::string& LedgerdServiceConfig::get_root_directory() const {
    return root_directory_;
}

void LedgerdServiceConfig::set_grpc_address(const std::string& grpc_address) {
    this->grpc_address_ = grpc_address;
}

const std::string& LedgerdServiceConfig::get_grpc_address() const {
    return grpc_address_;
}

void LedgerdServiceConfig::set_grpc_cluster_address(const std::string& grpc_cluster_address) {
    this->grpc_cluster_address_ = grpc_cluster_address;
}

const std::string& LedgerdServiceConfig::grpc_cluster_address() const {
    return grpc_cluster_address_;
}

void LedgerdServiceConfig::set_cluster_node_id(uint32_t node_id) {
    this->cluster_node_id_ = node_id;
}

const uint32_t LedgerdServiceConfig::cluster_node_id() const {
    return cluster_node_id_;
}

void LedgerdServiceConfig::add_node(uint32_t node_id, const std::string& host_and_port) {
    NodeInfo node_info(host_and_port);
    node_info_[node_id] = std::move(node_info);
}

const std::map<uint32_t, NodeInfo>& LedgerdServiceConfig::node_info() const {
    return node_info_;
}
}
