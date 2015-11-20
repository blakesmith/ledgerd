#include <gtest/gtest.h>
#include <ctime>

#include "paxos/group.h"
#include "paxos/memory_log_dispatcher.h"
#include "paxos/memory_message_dispatcher.h"

using namespace ledgerd::paxos;

namespace paxos_test {

TEST(Paxos, GroupMembership) {
    MemoryMessageDispatcher<std::string> message_dispatch;
    MemoryLogDispatcher<std::string> log_dispatch;
    Group<std::string> group1(&message_dispatch, &log_dispatch);
    Group<std::string> group2(&message_dispatch, &log_dispatch);
    Instance<AdminMessage>* instance = group1.AddNode(0);
    EXPECT_EQ(0, instance->sequence());
    EXPECT_EQ(0, instance->value()->node_id());
    EXPECT_EQ(InstanceRole::PROPOSER, instance->role());
    EXPECT_EQ(AdminMessageType::JOIN, instance->value()->message_type());
    EXPECT_EQ(nullptr, group1.node(0));
    time_t current_time = 0;

    group1.Tick(current_time);
}
}
