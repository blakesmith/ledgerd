#ifndef LEDGERD_PAXOS_INSTANCE_H_
#define LEDGERD_PAXOS_INSTANCE_H_

#include <vector>

#include "message.h"
#include "round.h"

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
    Round<T> round_;
    ProposalId highest_promise_;
    uint64_t sequence_;
    uint32_t this_node_id_;
    std::vector<uint32_t> node_ids_;
    std::unique_ptr<T> value_;

    ProposalId next_proposal() {
        round_.NextRound();
        return ProposalId(this_node_id_, round_.round_n());
    }

    Message<T> make_response(MessageType type, const Message<T>& request, const T* value) {
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

    void handle_prepare(const Message<T>& message, std::vector<Message<T>>* responses) {
        if(message.proposal_id() > highest_promise_) {
            set_highest_promise(message.proposal_id());
            responses->push_back(
                make_response(MessageType::PROMISE, message));
            transition(InstanceState::PROMISED);
        } else {
            responses->push_back(
                make_response(MessageType::REJECT, message, value_.get()));
        }
    }

    void handle_promise(const Message<T>& message, std::vector<Message<T>>* responses) {
        round_.AddPromise(message.source_node_id(),
                          message.proposal_id(),
                          message.value());
        const T* accept_value = round_.highest_value() ?
            round_.highest_value() :
            value_.get();
        if(round_.IsQuorum()) {
            responses->push_back(
                make_response(MessageType::ACCEPT, message, accept_value));
            if(value_ == nullptr) {
                value_ = std::unique_ptr<T>(new T(*accept_value));
            }
            transition(InstanceState::COMPLETE);
        }
    }

    void handle_accept(const Message<T>& message, std::vector<Message<T>>* responses) {
        if(message.proposal_id() >= highest_promise_) {
            value_ = std::unique_ptr<T>(new T(*message.value()));
            responses->push_back(
                make_response(MessageType::ACCEPTED, message, message.value()));
            transition(InstanceState::COMPLETE);
        }
    }

    std::vector<Message<T>> receive_acceptor(const std::vector<Message<T>>& inbound) {
        std::vector<Message<T>> responses;
        for(auto& message : inbound) {
            switch(state_) {
                case InstanceState::IDLE:
                case InstanceState::PROMISED:
                    switch(message.message_type()) {
                        case MessageType::PREPARE:
                            handle_prepare(message, &responses);
                            break;
                        case MessageType::ACCEPT:
                            handle_accept(message, &responses);
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
        std::vector<Message<T>> responses;
        for(auto& message : inbound) {
            switch(state_) {
                case InstanceState::PREPARING:
                    switch(message.message_type()) {
                        case MessageType::PROMISE:
                            handle_promise(message, &responses);
                            break;
                        default:
                            break;
                    }

                    break;
            }
        }

        return responses;
    }

public:
    Instance(InstanceRole role,
             uint64_t sequence,
             uint32_t this_node_id,
             std::vector<uint32_t> node_ids)
        : role_(role),
          state_(InstanceState::IDLE),
          highest_promise_(ProposalId(0, 0)),
          sequence_(sequence),
          this_node_id_(this_node_id),
          round_(node_ids.size()),
          node_ids_(node_ids),
          value_(nullptr) { }

    InstanceRole role() const {
        return role_;
    }

    void set_role(InstanceRole new_role) {
        this->role_ = new_role;
    }

    uint64_t sequence() const {
        return sequence_;
    }

    const Round<T>& round() const {
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

    void transition(InstanceState state) {
        this->state_ = state;
    }

    std::vector<Message<T>> Prepare(std::unique_ptr<T> value) {
        value_ = std::move(value);
        set_role(InstanceRole::PROPOSER);
        std::vector<Message<T>> messages = {
            Message<T>(MessageType::PREPARE,
                       sequence_,
                       next_proposal(),
                       this_node_id_,
                       node_ids_)
        };
        transition(InstanceState::PREPARING);
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

    T* final_value() const {
        if(state_ == InstanceState::COMPLETE) {
            return value_.get();
        }
        return nullptr;
    }
};
}
}

#endif
