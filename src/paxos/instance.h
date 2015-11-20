#ifndef LEDGERD_PAXOS_INSTANCE_H_
#define LEDGERD_PAXOS_INSTANCE_H_

namespace ledgerd {
namespace paxos {

enum InstanceRole {
    PROPOSER,
    ACCEPTOR,
    LISTENER
};

enum InstanceState {
    IDLE,
    PROPOSING,
    PROMISED,
    ACCEPTING,
    COMPLETE
};

template <typename T>
class Instance {
    InstanceRole role_;
    InstanceState state_;
    uint64_t sequence_;
    std::unique_ptr<T> value_;
public:
    Instance(InstanceRole role,
             uint64_t sequence,
             std::unique_ptr<T> value)
        : role_(role),
          state_(InstanceState::IDLE),
          sequence_(sequence),
          value_(std::move(value)) { }

    Instance(InstanceRole role,
             uint64_t sequence)
        : Instance(role,
                   sequence,
                   std::unique_ptr<T>(nullptr)) { }

    InstanceRole role() const {
        return role_;
    }

    uint64_t sequence() const {
        return sequence_;
    }

    T* value() const {
        return value_.get();
    }

    void Transition(InstanceState state) {
        this->state_ = state;
    }

    InstanceState state() const {
        return state_;
    }
};
}
}

#endif
