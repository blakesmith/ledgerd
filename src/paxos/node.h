#ifndef LEDGERD_PAXOS_NODE_H_
#define LEDGERD_PAXOS_NODE_H_

#include <map>

#include "instance.h"

namespace ledgerd {
namespace paxos {
template <typename T>
class Node {
    std::map<uint32_t, Instance<T>> active_instances;
};
}
}

#endif
