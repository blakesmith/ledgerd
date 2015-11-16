#ifndef LEDGERD_PAXOS_INSTANCE_H_
#define LEDGERD_PAXOS_INSTANCE_H_

namespace ledgerd {
namespace paxos {
template <typename T>
class Instance {
    T value;
};
}
}

#endif
