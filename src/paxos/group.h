#ifndef LEDGERD_PAXOS_GROUP_H_
#define LEDGERD_PAXOS_GROUP_H_

#include <ctime>
#include <map>
#include <memory>

#include "event.h"
#include "instance.h"
#include "node.h"

namespace ledgerd {
namespace paxos {

template <typename T>
class Group {
    uint32_t this_node_id_;
    uint64_t last_sequence_;
    std::time_t last_tick_time_;
    std::map<uint32_t, std::unique_ptr<Node<T>>> nodes_;
    std::map<uint64_t, std::unique_ptr<Instance<T>>> instances_;

    uint64_t next_sequence() {
        return last_sequence_++;
    }

public:
    Group(uint32_t this_node_id)
        : this_node_id_(this_node_id),
          last_sequence_(0),
          last_tick_time_(0) {
    }

    Node<T>* node(uint32_t node_id) const {
        auto search = nodes_.find(node_id);
        if(search != nodes_.end()) {
            return search->second.get();
        }

        return nullptr;
    }

    Node<T>* AddNode(uint32_t node_id) {
        std::unique_ptr<Node<T>> new_node(
            new Node<T>(node_id));
        Node<T>* node_ref = new_node.get();
        nodes_[node_id] = std::move(new_node);
        return node_ref;
    }

    void RemoveNode(uint32_t node_id) {
        nodes_.erase(node_id);
    }

    Instance<T>* CreateInstance(uint64_t sequence) {
        std::vector<uint32_t> instance_nodes;
        for(auto& kv : nodes_) { instance_nodes.push_back(kv.first); }
        std::unique_ptr<Instance<T>> new_instance(
            new Instance<T>(InstanceRole::ACCEPTOR,
                            sequence,
                            this_node_id_,
                            instance_nodes));
        Instance<T>* instance = new_instance.get();
        instances_[sequence] = std::move(new_instance);
        return instance;
    }

    Instance<T>* CreateInstance() {
        return CreateInstance(next_sequence());
    }

    Event<T> Propose(uint64_t sequence, std::unique_ptr<T> value) {
        auto search = instances_.find(sequence);
        if(search == instances_.end()) {
            return Event<T>();
        }
        const std::unique_ptr<Instance<T>>& instance = search->second;
        instance->set_proposed_value(std::move(value));
        return Event<T>(instance->Prepare());
    }

    Event<T> Receive(uint64_t sequence, const std::vector<Message<T>>& messages) {
        auto search = instances_.find(sequence);
        Instance<T>* instance;
        if(search == instances_.end()) {
            instance = CreateInstance(sequence);
        } else {
            instance = search->second.get();
        }
        std::vector<Message<T>> received_messages = instance->ReceiveMessages(messages);
        if(instance->state() == InstanceState::COMPLETE) {
            return Event<T>(instance->final_value());
        }

        return Event<T>(std::move(received_messages));
    }

    void Tick(std::time_t current_time) {
        this->last_tick_time_ = current_time;
    }
};
}
}

#endif
