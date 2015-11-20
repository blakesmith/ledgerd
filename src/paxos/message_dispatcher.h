#ifndef LEDGERD_PAXOS_MESSAGE_DISPATCHER_H_
#define LEDGERD_PAXOS_MESSAGE_DISPATCHER_H_

#include <memory>

#include "message.h"
#include "node.h"

namespace ledgerd {
namespace paxos {

template <typename T, typename C>
class MessageDispatcher {
public:
    virtual int Connect(Node<T, C>* node) = 0;
    virtual void Disconnect(Node<T, C>* node) = 0;
    virtual int SendMessage(Node<T, C>* dst_node, const Message<T>& message) = 0;
    virtual void ReceiveMessage(Node<T, C>* src_node, std::unique_ptr<Message<T>> message) = 0;

    virtual int SendAdminMessage(Node<T, C>* dst_node, const AdminMessage& message) = 0;
    virtual void ReceiveAdminMessage(Node<T, C>* src_node, std::unique_ptr<AdminMessage> message) = 0;
};
}
}

#endif
