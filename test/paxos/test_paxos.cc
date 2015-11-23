#include <gtest/gtest.h>
#include <ctime>

#include "paxos/group.h"
#include "paxos/memory_log_dispatcher.h"
#include "paxos/memory_message_dispatcher.h"

using namespace ledgerd::paxos;

namespace paxos_test {

TEST(Paxos, GroupMembership) {
    // MemoryMessageDispatcher<std::string, std::string> message_dispatch;
    // MemoryLogDispatcher<std::string> log_dispatch;
    // Group<std::string, std::string> group1(&message_dispatch, &log_dispatch, 0);
    // Group<std::string, std::string> group2(&message_dispatch, &log_dispatch, 1);

    // group1.ConnectPeers(std::map<uint32_t, std::string> {
    //         {0, "node1"}, {1, "node2"}
    //     });

    // group2.ConnectPeers(std::map<uint32_t, std::string> {
    //         {0, "node1"}, {1, "node2"}
    //     });

    // Instance<AdminMessage>* instance = group1.JoinGroup();
    // EXPECT_EQ(0, instance->sequence());
    // EXPECT_EQ(0, instance->value()->node_id());
    // EXPECT_EQ(InstanceRole::PROPOSER, instance->role());
    // EXPECT_EQ(AdminMessageType::JOIN, instance->value()->message_type());
    // ASSERT_TRUE(group1.node(0) != nullptr);
    // ASSERT_TRUE(group1.node(1) != nullptr);
    // ASSERT_TRUE(group2.node(0) != nullptr);
    // ASSERT_TRUE(group2.node(1) != nullptr);

    // time_t current_time = 0;
    // group1.Tick(current_time);
}

TEST(Paxos, InstancePromise) {
    std::vector<uint32_t> current_node_ids = {0, 1};
    Instance<std::string> i1(InstanceRole::ACCEPTOR, 0, 0, current_node_ids);
    Instance<std::string> i2(InstanceRole::ACCEPTOR, 0, 1, current_node_ids);
    EXPECT_EQ(InstanceState::IDLE, i1.state());
    EXPECT_EQ(InstanceState::IDLE, i2.state());

    std::vector<Message<std::string>> messages = i1.Prepare();
    EXPECT_EQ(InstanceRole::PROPOSER, i1.role());
    EXPECT_EQ(InstanceState::PREPARING, i1.state());
    ASSERT_EQ(1, messages.size());

    const Message<std::string>& message = messages[0];
    EXPECT_EQ(MessageType::PREPARE, message.message_type());
    const ProposalId expected_proposal(0, 2);
    EXPECT_EQ(expected_proposal, message.proposal_id());
    const std::vector<uint32_t> expected_node_ids {0, 1};
    EXPECT_EQ(expected_node_ids, message.target_node_ids());
    EXPECT_EQ(nullptr, message.value());

    std::vector<Message<std::string>> responses = i2.ReceiveMessages(messages);
    ASSERT_EQ(1, responses.size());
    const Message<std::string>& response = responses[0];
    EXPECT_EQ(MessageType::PROMISE, response.message_type());
    EXPECT_EQ(nullptr, response.value());
    const std::vector<uint32_t> expected_response_targets { 0 };
    EXPECT_EQ(expected_response_targets, response.target_node_ids());
}
}
