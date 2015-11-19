#ifndef LEDGERD_PAXOS_GROUP_H_
#define LEDGERD_PAXOS_GROUP_H_

#include <map>
#include <memory>

#include "instance.h"
#include "message_dispatcher.h"
#include "log_dispatcher.h"
#include "node.h"

namespace ledgerd {
namespace paxos {

template <typename T>
class Group {
    uint64_t last_sequence;
    std::map<uint32_t, std::unique_ptr<Node<T>>> nodes;
    MessageDispatcher<T>* message_dispatcher_;
    LogDispatcher<T>* log_dispatcher_;

    uint64_t next_sequence() const {
        if(last_sequence == 0) {
            return 0;
        }

        return last_sequence + 1;
    }

public:
    Group(MessageDispatcher<T>* message_dispatcher,
          LogDispatcher<T>* log_dispatcher)
        : last_sequence(0),
          message_dispatcher_(message_dispatcher),
          log_dispatcher_(log_dispatcher) { }

    LogDispatcher<T>* log_dispatcher() const {
        return log_dispatcher_;
    }

    MessageDispatcher<T>* message_dispatcher() const {
        return message_dispatcher_;
    }

    Node<T>* node(uint32_t node_id) const {
        auto search = nodes.find(node_id);
        if(search != nodes.end()) {
            return search->second.get();
        }

        return nullptr;
    }

    Instance<AdminMessage>* AddNode(uint32_t node_id) {
        AdminMessage message(node_id, AdminMessageType::JOIN);
        std::unique_ptr<Instance<AdminMessage>> instance(
            new Instance<AdminMessage>(next_sequence(), message));
        Instance<AdminMessage>* instance_ref = instance.get();
        return instance_ref;
    }

    void ConvergeOn(Instance<AdminMessage>* admin_message) {
    }
};
}
}

#endif
