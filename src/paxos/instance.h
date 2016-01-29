#ifndef LEDGERD_PAXOS_INSTANCE_H_
#define LEDGERD_PAXOS_INSTANCE_H_

#include <chrono>
#include <mutex>
#include <vector>

#include "message.h"
#include "round.h"
#include "value.h"

namespace ledgerd {
namespace paxos {

enum InstanceRole {
    PROPOSER,
    ACCEPTOR,
    LISTENER
};

enum InstanceState {
    IDLE,
    REJECTED,
    PREPARING,
    PROMISED,
    ACCEPTING,
    COMPLETE
};

template <typename T>
class Instance {
    static const int DEFAULT_RECEIVE_TIMEOUT = 200;

    std::mutex lock_;
    InstanceRole role_;
    InstanceState state_;
    Round<T> round_;
    ProposalId highest_promise_;
    uint64_t sequence_;
    uint32_t this_node_id_;
    bool carry_proposed_value_;
    std::vector<uint32_t> node_ids_;
    Value<T> proposed_value_;
    std::unique_ptr<T> final_value_;
    std::chrono::system_clock::time_point last_receive_;
    const std::chrono::system_clock::duration receive_timeout_;

    void set_role(InstanceRole new_role) {
        this->role_ = new_role;
    }

    void set_highest_promise(const ProposalId& id) {
        this->highest_promise_ = id;
    }

    void transition(InstanceState state) {
        this->state_ = state;
    }

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

    Message<T> make_accept_broadcast(MessageType type, const Message<T>& request, const T* value) {
        std::vector<uint32_t> target_nodes = round_.TargetAcceptNodes();
        return Message<T>(type,
                          sequence_,
                          request.proposal_id(),
                          this_node_id_,
                          target_nodes,
                          value);
    }

    Message<T> make_decided_broadcast(MessageType type, const Message<T>& request, const T* value) {
        std::vector<uint32_t> target_nodes = round_.TargetDecidedNodes();
        return Message<T>(type,
                          sequence_,
                          request.proposal_id(),
                          this_node_id_,
                          target_nodes,
                          value);
    }

    void handle_prepare(const Message<T>& message, std::vector<Message<T>>* responses) {
        if(message.proposal_id() > highest_promise_) {
            set_highest_promise(message.proposal_id());
            responses->push_back(
                make_response(MessageType::PROMISE, message, final_value_.get()));
            transition(InstanceState::PROMISED);
        } else {
            responses->push_back(
                make_response(MessageType::REJECT, message));
        }
    }

    void handle_promise(const Message<T>& message, std::vector<Message<T>>* responses) {
        round_.AddPromise(message.source_node_id(),
                          message.proposal_id(),
                          message.value());
        T* accept_value;
        if(final_value_) {
            accept_value = final_value_.get();
        } else {
            if(round_.highest_value()) {
                carry_proposed_value_ = true;
                accept_value = const_cast<T*>(round_.highest_value());
            } else {
                accept_value = const_cast<T*>(proposed_value_.value());
            }
        }
        if(round_.IsPromiseQuorum() || final_value_) {
            responses->push_back(
                make_accept_broadcast(MessageType::ACCEPT, message, accept_value));
            transition(InstanceState::ACCEPTING);
        }
    }

    void handle_accept(const Message<T>& message, std::vector<Message<T>>* responses) {
        if(message.proposal_id() >= highest_promise_) {
            responses->push_back(
                make_response(MessageType::ACCEPTED, message, message.value()));
            transition(InstanceState::ACCEPTING);
        } else {
            responses->push_back(
                make_response(MessageType::REJECT, message));
        }
    }

    void handle_accepted(const Message<T>& message, std::vector<Message<T>>* responses) {
        round_.AddAccepted(message.source_node_id(),
                           message.proposal_id(),
                           message.value());
        if(round_.IsAcceptQuorum()) {
            final_value_ = std::unique_ptr<T>(new T(*message.value()));
        }
        if(round_.IsAcceptQuorum() || final_value_) {
            responses->push_back(
                make_decided_broadcast(MessageType::DECIDED, message, final_value_.get()));
            transition(InstanceState::COMPLETE);
        }
    }

    void handle_decided(const Message<T>& message, std::vector<Message<T>>* responses) {
        final_value_ = std::unique_ptr<T>(new T(*message.value()));
        transition(InstanceState::COMPLETE);
    }

    void handle_rejected(const Message<T>& message, std::vector<Message<T>>* responses) {
        transition(InstanceState::REJECTED);
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
          carry_proposed_value_(false),
          round_(node_ids.size()),
          node_ids_(node_ids),
          proposed_value_(Value<T>(0, nullptr)),
          final_value_(nullptr),
          receive_timeout_(std::chrono::milliseconds(DEFAULT_RECEIVE_TIMEOUT)) { }

    InstanceRole role() {
        std::lock_guard<std::mutex> lock(lock_);
        return role_;
    }

    uint64_t sequence() const {
        return sequence_;
    }

    const Round<T>& round() {
        std::lock_guard<std::mutex> lock(lock_);
        return round_;
    }

    ProposalId highest_promise() {
        std::lock_guard<std::mutex> lock(lock_);
        return highest_promise_;
    }

    const std::vector<uint32_t>& node_ids() const {
        return node_ids_;
    }

    std::vector<Message<T>> Prepare() {
        std::lock_guard<std::mutex> lock(lock_);
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

    std::vector<Message<T>> ReceiveMessages(const std::vector<Message<T>>& inbound,
                                            std::chrono::time_point<std::chrono::system_clock> current_time = std::chrono::system_clock::now()) {
        std::lock_guard<std::mutex> lock(lock_);
        last_receive_ = current_time;
        std::vector<Message<T>> responses;
        for(auto& message : inbound) {
            switch(message.message_type()) {
                case MessageType::PREPARE:
                    handle_prepare(message, &responses);
                    break;
                case MessageType::PROMISE:
                    handle_promise(message, &responses);
                    break;
                case MessageType::ACCEPT:
                    handle_accept(message, &responses);
                    break;
                case MessageType::ACCEPTED:
                    handle_accepted(message, &responses);
                    break;
                case MessageType::REJECT:
                    handle_rejected(message, &responses);
                    break;
                case MessageType::DECIDED:
                    handle_decided(message, &responses);
                    break;
                default:
                    break;
            }
        }

        return responses;
    }

    std::vector<Message<T>> Tick(int rand, std::chrono::system_clock::time_point current_time) {
        std::chrono::milliseconds rand_duration(rand);
        if(current_time > (last_receive_ + receive_timeout_ + rand_duration)) {
            if(proposed_value_.value()) {
                return Prepare();
            }
        }

        return std::vector<Message<T>>{};
    }

    InstanceState state() {
        std::lock_guard<std::mutex> lock(lock_);
        return state_;
    }

    T* final_value() {
        std::lock_guard<std::mutex> lock(lock_);
        return final_value_.get();
    }

    void set_final_value(std::unique_ptr<T> value) {
        std::lock_guard<std::mutex> lock(lock_);
        final_value_ = std::move(value);
    }

    Value<T>& proposed_value() {
        std::lock_guard<std::mutex> lock(lock_);
        return proposed_value_;
    }

    Value<T> moved_proposed_value() {
        std::lock_guard<std::mutex> lock(lock_);
        return std::move(proposed_value_);
    }

    void set_proposed_value(Value<T>&& proposed) {
        std::lock_guard<std::mutex> lock(lock_);
        proposed_value_ = std::move(proposed);        
    }

    void set_proposed_value(std::unique_ptr<T> proposed) {
        set_proposed_value(Value<T>(0, std::move(proposed)));
    }

    bool carry_proposed_value() const {
        return carry_proposed_value_;
    }
};
}
}

#endif
