#ifndef LEDGERD_PAXOS_GROUP_H_
#define LEDGERD_PAXOS_GROUP_H_

#include <map>
#include <memory>

#include "message_dispatcher.h"
#include "log_dispatcher.h"
#include "node.h"

namespace ledgerd {
namespace paxos {
template <typename T>
class Group {
    std::map<uint32_t, Node<T>> nodes;
    std::unique_ptr<MessageDispatcher<T>> message_dispatcher_;
    std::unique_ptr<LogDispatcher<T>> log_dispatcher_;
public:
    Group(std::unique_ptr<MessageDispatcher<T>> message_dispatcher,
          std::unique_ptr<LogDispatcher<T>> log_dispatcher)
        : message_dispatcher_(std::move(message_dispatcher)),
          log_dispatcher_(std::move(log_dispatcher)) { }
};
}
}

#endif
