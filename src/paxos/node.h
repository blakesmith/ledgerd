#ifndef LEDGERD_PAXOS_NODE_H_
#define LEDGERD_PAXOS_NODE_H_

#include <map>

#include "instance.h"

namespace ledgerd {
namespace paxos {
template <typename T>
class Node {
    uint32_t id_;
    std::map<uint32_t, Instance<T>> active_instances;
public:
    Node(uint32_t id)
        : id_(id) { }

    uint32_t id() const {
        return id_;
    }
};
}
}

#endif
