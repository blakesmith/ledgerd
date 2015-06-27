#ifndef LEDGERD_SERVICE_CONFIG_H_
#define LEDGERD_SERVICE_CONFIG_H_

#include <string>

namespace ledgerd {
class LedgerdServiceConfig {
    std::string root_directory_;
public:
    void set_root_directory(std::string& root_directory);
    const std::string& get_root_directory() const;
};
}

#endif
