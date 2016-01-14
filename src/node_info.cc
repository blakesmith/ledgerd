#include "node_info.h"

namespace ledgerd {

NodeInfo::NodeInfo(const std::string& host_and_port)
    : host_and_port_(host_and_port) { }

NodeInfo& NodeInfo::operator=(NodeInfo&& rhs) {
    std::swap(host_and_port_, rhs.host_and_port_);
    return *this;
}

NodeInfo& NodeInfo::operator=(const NodeInfo& rhs){
    host_and_port_ = rhs.host_and_port_;
    return *this;
}

const std::string& NodeInfo::host_and_port() const {
    return host_and_port_;
}

}
