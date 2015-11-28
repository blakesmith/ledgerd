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

TEST(Group, LeapFroggingProposers) {
}
}
