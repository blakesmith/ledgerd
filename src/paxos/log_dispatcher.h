#ifndef LEDGERD_PAXOS_LOG_DISPATCHER_H_
#define LEDGERD_PAXOS_LOG_DISPATCHER_H_

#include <memory>

#include "message.h"

namespace ledgerd {
namespace paxos {

template <typename T>
class LogDispatcher {
public:
    virtual int WriteMessage(const Message<T>& message) = 0;
    virtual std::unique_ptr<Message<T>> ReadMessage(uint64_t id) = 0;

    virtual int WriteAdminMessage(const AdminMessage& message) = 0;
    virtual std::unique_ptr<AdminMessage> ReadAdminMessage(uint64_t id) = 0;
};
}
}

#endif
