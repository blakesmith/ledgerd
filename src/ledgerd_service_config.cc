#include "ledgerd_service_config.h"

namespace ledgerd {
LedgerdServiceConfig::LedgerdServiceConfig()
    : root_directory_("/var/lib/ledgerd"),
      grpc_address_("0.0.0.0:50051") {}

void LedgerdServiceConfig::set_root_directory(const std::string& root_directory) {
    root_directory_ = root_directory;
}

const std::string& LedgerdServiceConfig::get_root_directory() const {
    return root_directory_;
}

void LedgerdServiceConfig::set_grpc_address(const std::string& grpc_address) {
    grpc_address_ = grpc_address;
}

const std::string& LedgerdServiceConfig::get_grpc_address() const {
    return grpc_address_;
}
}
