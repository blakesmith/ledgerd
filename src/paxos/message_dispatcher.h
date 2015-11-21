#ifndef LEDGERD_PAXOS_MESSAGE_DISPATCHER_H_
#define LEDGERD_PAXOS_MESSAGE_DISPATCHER_H_

#include <map>
#include <memory>
#include <queue>
#include <vector>

#include "message.h"
#include "node.h"

namespace ledgerd {
namespace paxos {

template <typename T, typename C>
class MessageDispatcher {
private:
     std::queue<Message<T>> received_messages_;
     std::queue<AdminMessage> received_admin_messages_;
protected:
    void QueueReceived(Message<T>&& message) {
        received_messages_.push(message);
    }
public:
    std::vector<Message<T>> PopReceived() {
        std::vector<Message<T>> messages;
        while(!received_messages_.empty()) {
            messages.push_back(received_messages_.front());
            received_messages_.pop();
        }
        return messages;
    }

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
