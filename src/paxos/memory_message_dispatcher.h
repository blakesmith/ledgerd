#ifndef LEDGERD_PAXOS_MEMORY_MESSAGE_DISPATCHER_H_
#define LEDGERD_PAXOS_MEMORY_MESSAGE_DISPATCHER_H_

#include "message_dispatcher.h"

namespace ledgerd {
namespace paxos {

template <typename T>
class MemoryMessageDispatcher : public MessageDispatcher<T> {
public:
    int SendMessage(uint32_t node_id_dst, const Message<T>& message) override {
        return 0;
    }

    void ReceiveMessage(uint32_t node_id_src, std::unique_ptr<Message<T>> message) override {
    }

    int SendAdminMessage(uint32_t node_id_dst, const AdminMessage& message) override {
        return 0;
    }

    void ReceiveAdminMessage(uint32_t node_id_src, std::unique_ptr<AdminMessage> message) override {
    }
};

}
}

#endif
