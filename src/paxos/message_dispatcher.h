#ifndef LEDGERD_PAXOS_MESSAGE_DISPATCHER_H_
#define LEDGERD_PAXOS_MESSAGE_DISPATCHER_H_

#include <memory>

#include "message.h"

namespace ledgerd {
namespace paxos {

template <typename T>
class MessageDispatcher {
public:
    virtual int SendMessage(const Message<T>& message) = 0;
    virtual void ReceiveMessage(std::unique_ptr<Message<T>> message) = 0;

    virtual int SendAdminMessage(const AdminMessage& message) = 0;
    virtual void ReceiveAdminMessage(std::unique_ptr<AdminMessage> message) = 0;
};
}
}

#endif
