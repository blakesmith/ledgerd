#ifndef LEDGERD_PAXOS_GROUP_H_
#define LEDGERD_PAXOS_GROUP_H_

#include <ctime>
#include <map>
#include <memory>

#include "instance.h"
#include "message_dispatcher.h"
#include "log_dispatcher.h"
#include "node.h"

namespace ledgerd {
namespace paxos {

template <typename T, typename C>
class Group {
    uint64_t last_sequence_;
    uint32_t node_id_;
    std::time_t last_tick_time_;
    std::map<uint32_t, C> peer_data_;
    std::map<uint32_t, std::unique_ptr<Node<T, C>>> nodes_;
    std::map<uint64_t, std::unique_ptr<Instance<AdminMessage>>> admin_instances_;
    MessageDispatcher<T, C>* message_dispatcher_;
    LogDispatcher<T>* log_dispatcher_;

    uint64_t next_sequence() const {
        if(last_sequence_ == 0) {
            return 0;
        }

        return last_sequence_ + 1;
    }

    void broadcast(AdminMessage* message) {
        for(auto& node : nodes_) {
            message_dispatcher_->SendAdminMessage(node.second.get(), *message);
        }
    }

    void dispatch_messages() {
        for(auto& instance : admin_instances_) {
            switch(instance.second->state()) {
                case InstanceState::PROPOSING:
                    broadcast(instance.second->value());
                    break;
                default:
                    break;
            }
        }
    }

    void receive_messages() {
    }

public:
    Group(MessageDispatcher<T, C>* message_dispatcher,
          LogDispatcher<T>* log_dispatcher,
          uint32_t node_id)
        : last_tick_time_(0),
          last_sequence_(0),
          message_dispatcher_(message_dispatcher),
          log_dispatcher_(log_dispatcher),
          node_id_(node_id) { }

    LogDispatcher<T>* log_dispatcher() const {
        return log_dispatcher_;
    }

    MessageDispatcher<T, C>* message_dispatcher() const {
        return message_dispatcher_;
    }

    Node<T, C>* node(uint32_t node_id) const {
        auto search = nodes_.find(node_id);
        if(search != nodes_.end()) {
            return search->second.get();
        }

        return nullptr;
    }

    Instance<AdminMessage>* JoinGroup() {
        std::unique_ptr<AdminMessage> message(
            new AdminMessage(node_id_, AdminMessageType::JOIN));
        std::unique_ptr<Instance<AdminMessage>> instance(
            new Instance<AdminMessage>(InstanceRole::PROPOSER,
                                       next_sequence(),
                                       std::move(message)));
        instance->Transition(InstanceState::PROPOSING);
        Instance<AdminMessage>* instance_ref = instance.get();
        admin_instances_[instance->sequence()] = std::move(instance);
        return instance_ref;
    }

    void ConnectPeers(const std::map<uint32_t, C>& peer_data) {
        this->peer_data_ = peer_data;
        for(auto& peer : peer_data) {
            uint32_t node_id = peer.first;
            const C& connect_data = peer.second;
            std::unique_ptr<Node<T, C>> new_node(
                new Node<T, C>(node_id, connect_data));
            Node<T, C>* node_ref = new_node.get();
            nodes_[node_id] = std::move(new_node);
            message_dispatcher_->Connect(node_ref);
        }
    }

    void Tick(std::time_t current_time) {
        this->last_tick_time_ = current_time;
        dispatch_messages();
        receive_messages();
    }
};
}
}

#endif
