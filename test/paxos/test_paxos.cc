#include <gtest/gtest.h>

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
    EXPECT_EQ(0, instance->value().node_id());
    EXPECT_EQ(AdminMessageType::JOIN, instance->value().message_type());
    EXPECT_EQ(nullptr, group1.node(0));
    group1.ConvergeOn(instance);
    Node<std::string>* node = group1.node(0);
    ASSERT_TRUE(node != nullptr);
    EXPECT_EQ(0, node->id());
}
}
