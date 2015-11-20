#ifndef LEDGERD_PAXOS_MEMORY_MESSAGE_DISPATCHER_H_
#define LEDGERD_PAXOS_MEMORY_MESSAGE_DISPATCHER_H_

#include "message_dispatcher.h"

namespace ledgerd {
namespace paxos {

template <typename T, typename C>
class MemoryMessageDispatcher : public MessageDispatcher<T, C> {
public:
    int Connect(Node<T, C>* node) override {
        return 0;
    }

    void Disconnect(Node<T, C>* node) override {
    }

    int SendMessage(Node<T, C>* dst_node, const Message<T>& message) override {
        return 0;
    }

    void ReceiveMessage(Node<T, C>* src_node, std::unique_ptr<Message<T>> message) override {
    }

    int SendAdminMessage(Node<T, C>* dst_node, const AdminMessage& message) override {
        return 0;
    }

    void ReceiveAdminMessage(Node<T, C>* src_node, std::unique_ptr<AdminMessage> message) override {
    }
};

}
}

#endif
