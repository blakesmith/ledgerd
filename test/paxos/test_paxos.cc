#include <gtest/gtest.h>

#include "paxos/group.h"
#include "paxos/memory_log_dispatcher.h"
#include "paxos/memory_message_dispatcher.h"

using namespace ledgerd::paxos;

namespace paxos_test {
TEST(Paxos, GroupMembership) {
    std::unique_ptr<MessageDispatcher<std::string>> message_dispatch(
        new MemoryMessageDispatcher<std::string>());
    std::unique_ptr<LogDispatcher<std::string>> log_dispatch(
        new MemoryLogDispatcher<std::string>());
    Group<std::string> group(std::move(message_dispatch),
                             std::move(log_dispatch));
}
}
