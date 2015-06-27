#include "ledgerd_service_config.h"

namespace ledgerd {
void LedgerdServiceConfig::set_root_directory(std::string& root_directory) {
    root_directory_ = root_directory;
}

const std::string& LedgerdServiceConfig::get_root_directory() const {
    return root_directory_;
}
}
