#include <gtest/gtest.h>
#include <chrono>
#include <map>
#include <iostream>

#include "paxos/group.h"

using namespace ledgerd::paxos;

namespace paxos_test {

template <typename T>
class NullLog : public PersistentLog<T> {
public:
    LogStatus Write(uint64_t sequence, const T* final_value) {
        return LogStatus::LOG_OK;
    }

    std::unique_ptr<T> Get(uint64_t sequence) {
        return nullptr;
    }

    uint64_t HighestSequence() {
        return 0;
    }
};

template <typename T>
class MemoryLog : public PersistentLog<T> {
    std::map<uint64_t, std::unique_ptr<T>> final_values_;
public:
    LogStatus Write(uint64_t sequence, const T* final_value) {
        final_values_[sequence] = std::unique_ptr<T>(final_value ?
                                                     new T(*final_value) :
                                                     nullptr);
        return LogStatus::LOG_OK;
    }

    std::unique_ptr<T> Get(uint64_t sequence) {
        auto search = final_values_.find(sequence);
        if(search == final_values_.end()) {
            return nullptr;
        }
        return std::unique_ptr<T>(search->second ?
                                  new T(*search->second) :
                                  nullptr);
    }

    uint64_t HighestSequence() {
        return final_values_.rbegin()->first;
    }
};

template <typename T>
static uint64_t complete_sequence(Group<T>& primary_group,
                                  std::vector<Group<T>*> peers,
                                  std::unique_ptr<T> value) {
    Instance<T>* instance = primary_group.CreateInstance();
    auto broadcast_messages = primary_group.Propose(instance->sequence(), std::move(value));

    uint64_t sequence = instance->sequence();
    while(!primary_group.instance_complete(sequence)) {
        std::vector<Message<T>> replies;
        for(auto& g : peers) {
            for(auto& m : g->Receive(sequence, broadcast_messages)) {
                replies.push_back(m);
            }
        }
        broadcast_messages = primary_group.Receive(sequence, replies);
    }
    
    return instance->sequence();
}

template <typename T>
static std::vector<Message<T>> rpc(Group<T>& from,
                                   Group<T>& to,
                                   uint64_t sequence,
                                   const std::vector<Message<T>>& messages,
                                   std::chrono::time_point<std::chrono::system_clock> current_time = std::chrono::system_clock::now()) {
    auto response = to.Receive(sequence, messages, current_time);
    return from.Receive(sequence, response, current_time);
}

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

TEST(Group, OldProposal) {
    MemoryLog<std::string> log;
    Group<std::string> group1(0, log);
    Group<std::string> group2(1, log);
    Group<std::string> group3(2, log);

    group1.AddNode(0);
    group1.AddNode(1);
    group1.AddNode(2);

    group2.AddNode(0);
    group2.AddNode(1);
    group2.AddNode(2);

    group3.AddNode(0);
    group3.AddNode(1);
    group3.AddNode(2);

    std::vector<Group<std::string>*> groups { &group2, &group3 };
    std::unique_ptr<std::string> value(new std::string("hello"));
    uint64_t sequence = complete_sequence(group1,
                                          groups,
                                          std::move(value));
    ASSERT_TRUE(group1.final_value(sequence) != nullptr);
    EXPECT_EQ("hello", *group1.final_value(sequence));

    Group<std::string> group4(3, log);
    std::unique_ptr<std::string> value2(new std::string("join"));
    std::vector<Group<std::string>*> groups2 { &group1, &group2, &group3 };
    uint64_t sequence2 = complete_sequence(group4,
                                           groups2,
                                           std::move(value2));
    ASSERT_EQ(sequence, sequence2);
    ASSERT_TRUE(group4.final_value(sequence) != nullptr);
    EXPECT_EQ("hello", *group4.final_value(sequence2));
}

TEST(Group, GroupRestarts) {
    MemoryLog<std::string> log;
    Group<std::string> group1(0, log);
    Group<std::string> group2(1, log);
    Group<std::string> group3(2, log);

    group1.AddNode(0);
    group1.AddNode(1);
    group1.AddNode(2);

    group2.AddNode(0);
    group2.AddNode(1);
    group2.AddNode(2);

    group3.AddNode(0);
    group3.AddNode(1);
    group3.AddNode(2);

    std::vector<Group<std::string>*> groups { &group2, &group3 };
    std::unique_ptr<std::string> value1(new std::string("hello"));
    std::unique_ptr<std::string> value2(new std::string("there"));
    std::unique_ptr<std::string> value3(new std::string("friend"));
    uint64_t sequence1 = complete_sequence(group1,
                                           groups,
                                           std::move(value1));
    uint64_t sequence2 = complete_sequence(group1,
                                           groups,
                                           std::move(value2));
    uint64_t sequence3 = complete_sequence(group1,
                                           groups,
                                           std::move(value3));
    EXPECT_TRUE(group1.instance_complete(sequence1));
    EXPECT_TRUE(group1.instance_complete(sequence2));
    EXPECT_TRUE(group1.instance_complete(sequence3));

    Group<std::string> restarted_group(0, log);
    restarted_group.Start();
    EXPECT_TRUE(restarted_group.instance_complete(sequence1));
    EXPECT_TRUE(restarted_group.instance_complete(sequence2));
    EXPECT_TRUE(restarted_group.instance_complete(sequence3));
}

TEST(Group, LeapFroggingProposersTimeouts) {
    NullLog<std::string> log;
    Group<std::string> group1(0, log);
    Group<std::string> group2(1, log);
    Group<std::string> group3(2, log);

    group1.AddNode(0);
    group1.AddNode(1);
    group1.AddNode(2);

    group2.AddNode(0);
    group2.AddNode(1);
    group2.AddNode(2);

    group3.AddNode(0);
    group3.AddNode(1);
    group3.AddNode(2);

    std::unique_ptr<std::string> v1(new std::string("hello"));
    Instance<std::string>* i1 = group1.CreateInstance();

    std::unique_ptr<std::string> v2(new std::string("there"));
    Instance<std::string>* i2 = group2.CreateInstance();

    std::unique_ptr<std::string> v3(new std::string("friend"));
    Instance<std::string>* i3 = group3.CreateInstance();

    ASSERT_EQ(i1->sequence(), i2->sequence());
    ASSERT_EQ(i1->sequence(), i3->sequence());

    auto m1 = group3.Propose(i3->sequence(), std::move(v3));
    rpc(group3, group1, i3->sequence(), m1);
    rpc(group3, group2, i3->sequence(), m1);
    auto m2 = rpc(group3, group3, i3->sequence(), m1);

    auto m3 = group2.Propose(i2->sequence(), std::move(v2));
    rpc(group2, group1, i2->sequence(), m3);
    rpc(group2, group2, i2->sequence(), m3);
    auto m4 = group3.Receive(i2->sequence(), m3);
    ASSERT_EQ(1, m4.size());
    EXPECT_EQ(MessageType::REJECT, m4[0].message_type());

    auto m5 = group1.Propose(i1->sequence(), std::move(v1));
    std::chrono::time_point<std::chrono::system_clock> epoch;
    rpc(group1, group1, i1->sequence(), m5);
    rpc(group1, group2, i1->sequence(), m5, epoch);
    auto m6 = group3.Receive(i1->sequence(), m5);
    ASSERT_EQ(1, m6.size());
    EXPECT_EQ(MessageType::REJECT, m6[0].message_type());

    EXPECT_EQ(0, group2.Tick(epoch).size());
    EXPECT_EQ(1, group2.Tick(epoch + std::chrono::milliseconds(60)).size());
}
}
