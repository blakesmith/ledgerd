#ifndef LEDGERD_PAXOS_NODE_H_
#define LEDGERD_PAXOS_NODE_H_

#include <map>

#include "instance.h"

namespace ledgerd {
namespace paxos {
template <typename T, typename C>
class Node {
    uint32_t id_;
    const C& connect_data_;
    std::map<uint32_t, Instance<T>> active_instances;
public:
    Node(uint32_t id,
         const C& connect_data)
        : id_(id),
          connect_data_(connect_data) { }

    uint32_t id() const {
        return id_;
    }

    const C& connect_data() {
        return connect_data_;
    }
};
}
}

#endif
