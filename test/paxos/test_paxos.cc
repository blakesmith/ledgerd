#include <gtest/gtest.h>
#include <ctime>

#include "paxos/group.h"
#include "paxos/memory_log_dispatcher.h"
#include "paxos/memory_message_dispatcher.h"

using namespace ledgerd::paxos;

namespace paxos_test {

TEST(Paxos, GroupMembership) {
    MemoryMessageDispatcher<std::string, std::string> message_dispatch;
    MemoryLogDispatcher<std::string> log_dispatch;
    Group<std::string, std::string> group1(&message_dispatch, &log_dispatch, 0);
    Group<std::string, std::string> group2(&message_dispatch, &log_dispatch, 1);

    group1.ConnectPeers(std::map<uint32_t, std::string> {
            {0, "node1"}, {1, "node2"}
        });

    group2.ConnectPeers(std::map<uint32_t, std::string> {
            {0, "node1"}, {1, "node2"}
        });

    Instance<AdminMessage>* instance = group1.JoinGroup();
    EXPECT_EQ(0, instance->sequence());
    EXPECT_EQ(0, instance->value()->node_id());
    EXPECT_EQ(InstanceRole::PROPOSER, instance->role());
    EXPECT_EQ(AdminMessageType::JOIN, instance->value()->message_type());
    ASSERT_TRUE(group1.node(0) != nullptr);
    ASSERT_TRUE(group1.node(1) != nullptr);
    ASSERT_TRUE(group2.node(0) != nullptr);
    ASSERT_TRUE(group2.node(1) != nullptr);

    time_t current_time = 0;
    group1.Tick(current_time);
}
}
