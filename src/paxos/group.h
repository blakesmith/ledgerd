#ifndef LEDGERD_PAXOS_GROUP_H_
#define LEDGERD_PAXOS_GROUP_H_

#include <map>

#include "message.h"
#include "node.h"

namespace ledgerd {
namespace paxos {
template <typename T>
class Group {
    std::map<uint32_t, Node<T>> nodes;
};
}
}

#endif
