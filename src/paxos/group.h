#ifndef LEDGERD_PAXOS_GROUP_H_
#define LEDGERD_PAXOS_GROUP_H_

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <random>

#include "instance.h"
#include "linear_sequence.h"
#include "listener.h"
#include "log.h"
#include "node.h"
#include "persistent_log.h"

namespace ledgerd {
namespace paxos {

template <typename T,
          typename V = bool>
class Group {
    std::mutex lock_;
    uint32_t this_node_id_;
    PersistentLog<T>& persistent_log_;
    LinearSequence<uint64_t> active_or_completed_instances_;
    LinearSequence<uint64_t> completed_instances_;
    LinearSequence<uint64_t> journaled_instances_;
    LinearSequence<uint64_t, V> value_reads_;
    std::map<uint32_t, std::unique_ptr<Node<T>>> nodes_;
    std::map<uint64_t, std::unique_ptr<Instance<T>>> instances_;
    std::vector<Listener<T, V>*> listeners_;
    std::random_device random_;
    std::uniform_int_distribution<int> random_dist_;

    void notify_listeners(uint64_t sequence, const T* final_value) {
        for(auto listener : listeners_) {
            listener->Receive(sequence, final_value);
        }
    }

    void notify_reader(Instance<T>* instance) {
        const T* event_value = instance->proposed_value().value();
        if(event_value != nullptr) {
            V mapped_value;
            for(auto listener : listeners_) {
                auto status = listener->Map(event_value, &mapped_value);
                if(status == ListenerStatus::OK) {
                    value_reads_.Deliver(instance->proposed_value().id(),
                                         std::move(mapped_value));
                }
            }
        }
    }

    void persist_instances() {
        for(auto it = instances_.begin(); it != instances_.end(); ++it) {
            if(completed_instances_.in_joint_range(it->first) &&
               !journaled_instances_.in_joint_range(it->first)) {
                LEDGERD_LOG(logDEBUG) << "About to journal instance: " << it->first << " on node: " << this_node_id_;
                LogStatus status = persistent_log_.Write(it->second->sequence(),
                                                         it->second->final_value());
                if(status == LogStatus::LOG_OK) {
                    LEDGERD_LOG(logDEBUG) << "Journaling instance: " << it->first << " on node: " << this_node_id_;
                    journaled_instances_.Add(it->first);
                    notify_listeners(it->second->sequence(),
                                     it->second->final_value());
                } else {
                    LEDGERD_LOG(logDEBUG) << "Error journaling instance: " << it->first << " on node: " << this_node_id_;
                }
            }
        }
    }

    // This should only be run in the case where a listener is added after
    // sequences have already been written to the log
    void prime_listeners(uint64_t highest_log_sequence) {
        for(auto listener : listeners_) {
            uint64_t highest_listener_sequence = listener->HighestSequence();
            if(highest_listener_sequence < highest_log_sequence) {
                LEDGERD_LOG(logINFO) << "Listener is "
                                     << (highest_log_sequence - highest_listener_sequence)
                                     << " sequences behind, priming state";
            }
            while(highest_listener_sequence < highest_log_sequence) {
                uint64_t next_sequence = ++highest_listener_sequence;
                auto value = persistent_log_.Get(next_sequence);
                listener->Receive(next_sequence, value.get());
                highest_listener_sequence = next_sequence;
            }
        }
    }

    void prime_state() {
        uint64_t highest_log_sequence = persistent_log_.HighestSequence();
        completed_instances_.set_upper_bound(highest_log_sequence);
        prime_listeners(highest_log_sequence);
    }

    Instance<T>* create_instance(uint64_t sequence) {
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

    std::vector<Message<T>> propose(uint64_t sequence,
                                    Value<T> value) {
        auto search = instances_.find(sequence);
        if(search == instances_.end()) {
            return std::vector<Message<T>>{};
        }
        const std::unique_ptr<Instance<T>>& instance = search->second;
        instance->set_proposed_value(std::move(value));
        return instance->Prepare();
    }

public:
    Group(uint32_t this_node_id,
          PersistentLog<T>& persistent_log,
          std::uniform_int_distribution<int> random_dist = std::uniform_int_distribution<int>(0, 3))
        : this_node_id_(this_node_id),
          persistent_log_(persistent_log),
          random_dist_(random_dist),
          active_or_completed_instances_(0),
          completed_instances_(0),
          journaled_instances_(0),
          value_reads_(0) { }

    Node<T>* node(uint32_t node_id) {
        std::lock_guard<std::mutex> lock(lock_);
        auto search = nodes_.find(node_id);
        if(search != nodes_.end()) {
            return search->second.get();
        }

        return nullptr;
    }

    void Start() {
        std::lock_guard<std::mutex> lock(lock_);
        prime_state();
    }

    Node<T>* AddNode(uint32_t node_id) {
        std::lock_guard<std::mutex> lock(lock_);
        std::unique_ptr<Node<T>> new_node(
            new Node<T>(node_id));
        Node<T>* node_ref = new_node.get();
        nodes_[node_id] = std::move(new_node);
        return node_ref;
    }

    void AddListener(Listener<T, V>* listener) {
        std::lock_guard<std::mutex> lock(lock_);
        listeners_.push_back(listener);
    }

    void RemoveNode(uint32_t node_id) {
        std::lock_guard<std::mutex> lock(lock_);
        nodes_.erase(node_id);
    }

    Instance<T>* CreateInstance(uint64_t sequence) {
        std::lock_guard<std::mutex> lock(lock_);
        return create_instance(sequence);
    }

    Instance<T>* CreateInstance() {
        std::lock_guard<std::mutex> lock(lock_);
        return create_instance(active_or_completed_instances_.next());
    }

    std::vector<Message<T>> Propose(uint64_t sequence,
                                    std::unique_ptr<T> value,
                                    uint64_t* value_read_id = nullptr) {
        std::lock_guard<std::mutex> lock(lock_);
        uint64_t next_id = value_reads_.next();
        value_reads_.Add(next_id);
        Value<T> wrapped_value(next_id, std::move(value));
        if(value_read_id != nullptr) {
            *value_read_id = wrapped_value.id();
        }
        return propose(sequence, std::move(wrapped_value));
    }

    std::vector<Message<T>> Receive(uint64_t sequence,
                                    const std::vector<Message<T>>& messages,
                                    std::chrono::time_point<std::chrono::system_clock> current_time = std::chrono::system_clock::now()) {
        std::lock_guard<std::mutex> lock(lock_);
        auto search = instances_.find(sequence);
        Instance<T>* instance;
        if(search == instances_.end()) {
            instance = create_instance(sequence);
            std::unique_ptr<T> log_final_value = persistent_log_.Get(sequence);
            if(log_final_value) {
                instance->set_final_value(std::move(log_final_value));
            }
        } else {
            instance = search->second.get();
        }
        std::vector<Message<T>> received_messages = instance->ReceiveMessages(messages, current_time);

        if(instance->state() == InstanceState::COMPLETE) {
            completed_instances_.Add(instance->sequence());
            persist_instances();
            // We still have a proposed value that needs to
            // be completed, start another round of Paxos
            if(instance->carry_proposed_value()) {
                Instance<T>* next_instance = create_instance(active_or_completed_instances_.next());
                LEDGERD_LOG(logDEBUG) << "Next sequence is: " << next_instance->sequence();
                auto new_messages = propose(next_instance->sequence(),
                                            instance->moved_proposed_value());
                for(auto& m : new_messages) {
                    received_messages.push_back(std::move(m));
                }
            } else {
                notify_reader(instance);
            }
        }

        return received_messages;
    }

    std::vector<Message<T>> Tick(std::chrono::time_point<std::chrono::system_clock> current_time = std::chrono::system_clock::now()) {
        std::lock_guard<std::mutex> lock(lock_);
        int rand = random_dist_(random_);
        std::vector<Message<T>> messages;
        for(auto it = instances_.begin(); it != instances_.end(); ++it) {
            if(journaled_instances_.in_joint_range(it->first)) {
                LEDGERD_LOG(logDEBUG) << "Removing completed instance: " << it->first << " on node: " << this_node_id_;
                instances_.erase(it);
                break;
            }
            for(auto message : it->second->Tick(rand, current_time)) {
                messages.push_back(message);
            }
        }
        return messages;
    }

    void WaitForJournaled(uint64_t sequence) {
        journaled_instances_.WaitForUpperBound(sequence);
    }

    std::future<V> ReadValue(uint64_t value_id) {
        return value_reads_.Future(value_id);
    }

    void ClearValue(uint64_t value_id) {
        value_reads_.Clear(value_id);
    }

    std::unique_ptr<T> final_value(uint64_t sequence) {
        std::lock_guard<std::mutex> lock(lock_);
        return persistent_log_.Get(sequence);
    }

    bool instance_complete(uint64_t sequence) {
        std::lock_guard<std::mutex> lock(lock_);
        return completed_instances_.in_joint_range(sequence);
    }

    uint32_t id() const {
        return this_node_id_;
    }
};
}
}

#endif
