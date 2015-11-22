#ifndef LEDGERD_PAXOS_INSTANCE_H_
#define LEDGERD_PAXOS_INSTANCE_H_

#include <vector>

#include "message.h"

namespace ledgerd {
namespace paxos {

enum InstanceRole {
    PROPOSER,
    ACCEPTOR,
    LISTENER
};

enum InstanceState {
    IDLE,
    PREPARING,
    PROMISED,
    ACCEPTING,
    COMPLETE
};

template <typename T>
class Instance {
    InstanceRole role_;
    InstanceState state_;
    uint64_t sequence_;
    uint32_t round_;
    uint32_t this_node_id_;
    std::vector<uint32_t> node_ids_;
    std::unique_ptr<T> value_;

    ProposalId next_proposal() {
        // TODO: Unique proposal ids
        return ProposalId(this_node_id_, 0);
    }

public:
    Instance(InstanceRole role,
             uint64_t sequence,
             uint32_t this_node_id,
             std::vector<uint32_t> node_ids,
             std::unique_ptr<T> value)
        : role_(role),
          state_(InstanceState::IDLE),
          sequence_(sequence),
          this_node_id_(this_node_id),
          round_(0),
          node_ids_(node_ids),
          value_(std::move(value)) { }

    Instance(InstanceRole role,
             uint64_t sequence,
             uint32_t this_node_id,
             std::vector<uint32_t> node_ids)
        : Instance(role,
                   sequence,
                   this_node_id,
                   node_ids,
                   std::unique_ptr<T>(nullptr)) { }

    InstanceRole role() const {
        return role_;
    }

    void set_role(InstanceRole new_role) {
        this->role_ = new_role;
    }

    uint64_t sequence() const {
        return sequence_;
    }

    uint32_t round() const {
        return round_;
    }

    const std::vector<uint32_t>& node_ids() const {
        return node_ids_;
    }

    T* value() const {
        return value_.get();
    }

    void Transition(InstanceState state) {
        this->state_ = state;
    }

    std::vector<Message<T>> Prepare() {
        set_role(InstanceRole::PROPOSER);
        Transition(InstanceState::PREPARING);
        std::vector<Message<T>> messages = {
            Message<T>(MessageType::PREPARE,
                       next_proposal(),
                       node_ids_)
        };
        return messages;
    }

    InstanceState state() const {
        return state_;
    }
};
}
}

#endif
