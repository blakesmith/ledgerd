#ifndef LEDGERD_PAXOS_GROUP_H_
#define LEDGERD_PAXOS_GROUP_H_

#include <ctime>
#include <map>
#include <memory>

#include "instance.h"
#include "linear_sequence.h"
#include "node.h"
#include "persistent_log.h"

namespace ledgerd {
namespace paxos {

template <typename T>
class Group {
    uint32_t this_node_id_;
    PersistentLog<T>& persistent_log_;
    LinearSequence<uint64_t> active_or_completed_instances_;
    LinearSequence<uint64_t> completed_instances_;
    std::time_t last_tick_time_;
    std::map<uint32_t, std::unique_ptr<Node<T>>> nodes_;
    std::map<uint64_t, std::unique_ptr<Instance<T>>> instances_;

public:
    Group(uint32_t this_node_id,
          PersistentLog<T>& persistent_log)
        : this_node_id_(this_node_id),
          persistent_log_(persistent_log),
          active_or_completed_instances_(0),
          completed_instances_(0),
          last_tick_time_(0) { }

    Node<T>* node(uint32_t node_id) const {
        auto search = nodes_.find(node_id);
        if(search != nodes_.end()) {
            return search->second.get();
        }

        return nullptr;
    }

    Instance<T>* instance(uint64_t sequence) const {
        auto search = instances_.find(sequence);
        if(search != instances_.end()) {
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
        active_or_completed_instances_.Add(sequence);
        return instance;
    }

    Instance<T>* CreateInstance() {
        return CreateInstance(active_or_completed_instances_.next());
    }

    std::vector<Message<T>> Propose(uint64_t sequence, std::unique_ptr<T> value) {
        auto search = instances_.find(sequence);
        if(search == instances_.end()) {
            return std::vector<Message<T>>{};
        }
        const std::unique_ptr<Instance<T>>& instance = search->second;
        instance->set_proposed_value(std::move(value));
        return instance->Prepare();
    }

    std::vector<Message<T>> Receive(uint64_t sequence, const std::vector<Message<T>>& messages) {
        auto search = instances_.find(sequence);
        Instance<T>* instance;
        if(search == instances_.end()) {
            instance = CreateInstance(sequence);
            std::unique_ptr<T> log_final_value = persistent_log_.Get(sequence);
            if(log_final_value) {
                instance->set_final_value(std::move(log_final_value));
            }
        } else {
            instance = search->second.get();
        }
        std::vector<Message<T>> received_messages = instance->ReceiveMessages(messages);
        if(instance->state() == InstanceState::COMPLETE) {
            LogStatus status = persistent_log_.Write(instance->sequence(),
                                                     instance->final_value());
            if(status == LogStatus::LOG_OK) {
                instances_.erase(instance->sequence());
            }
            completed_instances_.Add(instance->sequence());
            return std::vector<Message<T>>{};
        }

        return received_messages;
    }

    void Tick(std::time_t current_time) {
        this->last_tick_time_ = current_time;
    }

    // Returns a copy, since this will eventually come from
    // the persistent log
    std::unique_ptr<T> final_value(uint64_t sequence) {
        auto search = instances_.find(sequence);
        if(search == instances_.end()) {
            return nullptr;
        }
        const std::unique_ptr<Instance<T>>& instance = search->second;
        T* final_value = instance->final_value();
        return std::unique_ptr<T>(final_value ?
                                  new T(*final_value) :
                                  nullptr);
    }

};
}
}

#endif
