#ifndef LEDGERD_PAXOS_MEMORY_LOG_DISPATCHER_H_
#define LEDGERD_PAXOS_MEMORY_LOG_DISPATCHER_H_

#include "log_dispatcher.h"

namespace ledgerd {
namespace paxos {

template <typename T>
class MemoryLogDispatcher : public LogDispatcher<T> {
public:
    int WriteMessage(const Message<T>& message) override {
        return 0;
    }

    std::unique_ptr<Message<T>> ReadMessage(uint64_t id) override {
        return std::unique_ptr<Message<T>>(nullptr);
    }

    int WriteAdminMessage(const AdminMessage& message) override {
        return 0;
    }

    std::unique_ptr<AdminMessage> ReadAdminMessage(uint64_t id) override {
        return std::unique_ptr<AdminMessage>(nullptr);
    }
};
}
}

#endif
