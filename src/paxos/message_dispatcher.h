#ifndef LEDGERD_PAXOS_MESSAGE_DISPATCHER_H_
#define LEDGERD_PAXOS_MESSAGE_DISPATCHER_H_

#include <memory>

#include "message.h"

namespace ledgerd {
namespace paxos {

template <typename T>
class MessageDispatcher {
public:
    virtual int SendMessage(uint32_t node_id_dst, const Message<T>& message) = 0;
    virtual void ReceiveMessage(uint32_t node_id_src, std::unique_ptr<Message<T>> message) = 0;

    virtual int SendAdminMessage(uint32_t node_id_dst, const AdminMessage& message) = 0;
    virtual void ReceiveAdminMessage(uint32_t node_id_src, std::unique_ptr<AdminMessage> message) = 0;
};
}
}

#endif
