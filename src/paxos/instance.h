#ifndef LEDGERD_PAXOS_INSTANCE_H_
#define LEDGERD_PAXOS_INSTANCE_H_

namespace ledgerd {
namespace paxos {
template <typename T>
class Instance {
    uint64_t sequence_;
    T value_;
public:
    Instance(uint64_t sequence,
             const T& value)
        : sequence_(sequence),
          value_(value) { }

    uint64_t sequence() const {
        return sequence_;
    }
    const T& value() const {
        return value_;
    }
};
}
}

#endif
