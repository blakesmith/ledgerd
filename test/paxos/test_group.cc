#include <gtest/gtest.h>
#include <ctime>

#include "paxos/group.h"

using namespace ledgerd::paxos;

namespace paxos_test {

TEST(Group, AddRemoveNode) {
    Group<std::string> group(0);
    Node<std::string>* added_node = group.AddNode(1);
    ASSERT_TRUE(added_node != nullptr);
    EXPECT_EQ(1, added_node->id());
    
    group.RemoveNode(1);
    EXPECT_TRUE(group.node(1) == nullptr);
}

TEST(Group, CreateInstance) {
    Group<std::string> group(0);
    group.AddNode(0);
    group.AddNode(1);
    
    const Instance<std::string>& instance = group.CreateInstance();
    EXPECT_EQ(0, instance.sequence());
    const std::vector<uint32_t> instance_nodes { 0, 1 };
    EXPECT_EQ(instance_nodes, instance.node_ids());
}

TEST(Group, PeerProposals) {
    Group<std::string> group1(0);
    Group<std::string> group2(1);

    group1.AddNode(0);
    group1.AddNode(1);
    group2.AddNode(0);
    group2.AddNode(1);

    std::unique_ptr<std::string> value(new std::string("hello"));
    const Instance<std::string>& instance = group1.CreateInstance();
    Event<std::string> event = group1.Propose(instance.sequence(), std::move(value));
    ASSERT_TRUE(event.HasMessages());
    EXPECT_FALSE(event.HasValue());
}

TEST(Group, LeapFroggingProposers) {
}
}
