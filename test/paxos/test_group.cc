#include <gtest/gtest.h>
#include <ctime>

#include "paxos/group.h"

using namespace ledgerd::paxos;

namespace paxos_test {

template <typename T>
class NullLog : public PersistentLog<T> {
public:
    LogStatus Write(const Instance<T>* instance) { }
};

TEST(Group, AddRemoveNode) {
    NullLog<std::string> log;
    Group<std::string> group(0, log);
    Node<std::string>* added_node = group.AddNode(1);
    ASSERT_TRUE(added_node != nullptr);
    EXPECT_EQ(1, added_node->id());
    
    group.RemoveNode(1);
    EXPECT_TRUE(group.node(1) == nullptr);
}

TEST(Group, CreateInstance) {
    NullLog<std::string> log;
    Group<std::string> group(0, log);
    group.AddNode(0);
    group.AddNode(1);
    
    Instance<std::string>* instance = group.CreateInstance();
    EXPECT_EQ(1, instance->sequence());
    const std::vector<uint32_t> instance_nodes { 0, 1 };
    EXPECT_EQ(instance_nodes, instance->node_ids());
}

TEST(Group, MultiplePeerProposals) {
    NullLog<std::string> log;
    Group<std::string> group1(0, log);
    Group<std::string> group2(1, log);

    group1.AddNode(0);
    group1.AddNode(1);
    group2.AddNode(0);
    group2.AddNode(1);

    std::unique_ptr<std::string> value(new std::string("hello"));
    Instance<std::string>* instance = group1.CreateInstance();
    EXPECT_EQ(1, instance->sequence());
    std::vector<Message<std::string>> messages = group1.Propose(instance->sequence(), std::move(value));
    ASSERT_EQ(1, messages.size());

    std::vector<Message<std::string>> messages2 = group2.Receive(instance->sequence(), messages);
    EXPECT_EQ(1, messages2.size());

    std::unique_ptr<std::string> value2(new std::string("there"));
    Instance<std::string>* instance2 = group2.CreateInstance();
    EXPECT_EQ(2, instance2->sequence());
    std::vector<Message<std::string>> messages3 = group2.Propose(instance2->sequence(), std::move(value2));
    EXPECT_EQ(1, messages3.size());

    std::vector<Message<std::string>> messages4 = group1.Receive(instance2->sequence(), messages3);
    EXPECT_EQ(1, messages4.size());
}

TEST(Group, LeapFroggingProposers) {
}
}
