#ifndef LEDGER_CLUSTER_NODE_INFO_H_
#define LEDGER_CLUSTER_NODE_INFO_H_

#include <string>

namespace ledgerd {

class NodeInfo {
    std::string host_and_port_;
public:
    NodeInfo() = default;
    NodeInfo(const std::string& host_and_port);
    NodeInfo(const NodeInfo& rhs) = default;
    NodeInfo(NodeInfo&& rhs) = default;
    ~NodeInfo() = default;
    NodeInfo& operator=(NodeInfo&& rhs);
    NodeInfo& operator=(const NodeInfo& rhs);

    const std::string& host_and_port() const;
};

}

#endif
