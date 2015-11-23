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
    ProposalId highest_promise_;
    uint64_t sequence_;
    uint32_t this_node_id_;
    uint32_t round_;
    std::vector<uint32_t> node_ids_;
    std::unique_ptr<T> value_;

    ProposalId next_proposal() {
        round_++;
        uint32_t prop_n = round_ * node_ids_.size() + this_node_id_;
        return ProposalId(this_node_id_, prop_n);
    }

    Message<T> make_response(MessageType type, const Message<T>& request, T* value) {
        std::vector<uint32_t> target_nodes { request.source_node_id() };
        return Message<T>(type,
                          sequence_,
                          request.proposal_id(),
                          this_node_id_,
                          target_nodes,
                          value);
    }

    Message<T> make_response(MessageType type, const Message<T>& request) {
        std::vector<uint32_t> target_nodes { request.source_node_id() };
        return Message<T>(type,
                          sequence_,
                          request.proposal_id(),
                          this_node_id_,
                          target_nodes);
    }

    std::vector<Message<T>> receive_acceptor(const std::vector<Message<T>>& inbound) {
        std::vector<Message<T>> responses;
        for(auto& message : inbound) {
            switch(state_) {
                case InstanceState::IDLE:
                case InstanceState::PROMISED:
                    switch(message.message_type()) {
                        case MessageType::PREPARE:
                            if(message.proposal_id() > highest_promise_) {
                                set_highest_promise(message.proposal_id());
                                responses.push_back(
                                    make_response(MessageType::PROMISE, message));
                                Transition(InstanceState::PROMISED);
                            } else {
                                responses.push_back(
                                    make_response(MessageType::REJECT, message, value_.get()));
                            }
                            break;
                        default:
                            break;
                    }
 
                    break;
            }
        }

        return responses;
    }

    std::vector<Message<T>> receive_proposer(const std::vector<Message<T>>& inbound) {
        return std::vector<Message<T>>();
    }

public:
    Instance(InstanceRole role,
             uint64_t sequence,
             uint32_t this_node_id,
             std::vector<uint32_t> node_ids,
             std::unique_ptr<T> value)
        : role_(role),
          state_(InstanceState::IDLE),
          highest_promise_(ProposalId(0, 0)),
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

    ProposalId highest_promise() const {
        return highest_promise_;
    }

    void set_highest_promise(const ProposalId& id) {
        this->highest_promise_ = id;
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
                       sequence_,
                       next_proposal(),
                       this_node_id_,
                       node_ids_)
        };
        Transition(InstanceState::PREPARING);
        return messages;
    }

    std::vector<Message<T>> ReceiveMessages(const std::vector<Message<T>>& inbound) {
        switch(role_) {
            case InstanceRole::ACCEPTOR:
                return receive_acceptor(inbound);
                break;
            case InstanceRole::PROPOSER:
                return receive_proposer(inbound);
                break;
        }
    }

    InstanceState state() const {
        return state_;
    }
};
}
}

#endif
