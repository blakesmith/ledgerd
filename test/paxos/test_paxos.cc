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

TEST(Paxos, InstanceSupercedingPromise) {
    std::unique_ptr<std::string> value(new std::string("hello"));
    std::vector<uint32_t> current_node_ids = {0, 1, 2};
    Instance<std::string> i1(InstanceRole::ACCEPTOR, 0, 0, current_node_ids);
    Instance<std::string> i2(InstanceRole::ACCEPTOR, 0, 1, current_node_ids);
    Instance<std::string> i3(InstanceRole::ACCEPTOR, 0, 2, current_node_ids);
    EXPECT_EQ(InstanceState::IDLE, i1.state());
    EXPECT_EQ(InstanceState::IDLE, i2.state());
    EXPECT_EQ(InstanceState::IDLE, i3.state());

    std::vector<Message<std::string>> messages = i1.Prepare(std::move(value));
    EXPECT_EQ(InstanceRole::PROPOSER, i1.role());
    EXPECT_EQ(InstanceState::PREPARING, i1.state());
    ASSERT_EQ(1, messages.size());

    const Message<std::string>& message = messages[0];
    EXPECT_EQ(MessageType::PREPARE, message.message_type());
    const ProposalId expected_proposal(0, 3);
    EXPECT_EQ(expected_proposal, message.proposal_id());
    const std::vector<uint32_t> expected_node_ids {0, 1, 2};
    EXPECT_EQ(expected_node_ids, message.target_node_ids());
    EXPECT_EQ(nullptr, message.value());

    std::vector<Message<std::string>> responses = i2.ReceiveMessages(messages);
    ASSERT_EQ(1, responses.size());
    const Message<std::string>& response = responses[0];
    EXPECT_EQ(MessageType::PROMISE, response.message_type());
    EXPECT_EQ(0, message.sequence());
    EXPECT_EQ(nullptr, response.value());
    const std::vector<uint32_t> expected_response_targets { 0 };
    EXPECT_EQ(expected_response_targets, response.target_node_ids());
    EXPECT_EQ(expected_proposal, i2.highest_promise());

    std::unique_ptr<std::string> value2(new std::string("there"));
    std::vector<Message<std::string>> late_prepare = i3.Prepare(std::move(value2));
    ASSERT_EQ(1, late_prepare.size());
    std::vector<Message<std::string>> rejects = i2.ReceiveMessages(late_prepare);
    ASSERT_EQ(1, rejects.size());
    const Message<std::string>& reject = rejects[0];
    EXPECT_EQ(MessageType::PROMISE, reject.message_type());
    const std::vector<uint32_t> expected_reject_targets { 2 };
    EXPECT_EQ(expected_reject_targets, reject.target_node_ids());
}

TEST(Paxos, InstanceRejectingPromise) {
    std::unique_ptr<std::string> value(new std::string("hello"));
    std::vector<uint32_t> current_node_ids = {0, 1, 2};
    Instance<std::string> i1(InstanceRole::ACCEPTOR, 0, 0, current_node_ids);
    Instance<std::string> i2(InstanceRole::ACCEPTOR, 0, 1, current_node_ids);
    Instance<std::string> i3(InstanceRole::ACCEPTOR, 0, 2, current_node_ids);
    EXPECT_EQ(InstanceState::IDLE, i1.state());
    EXPECT_EQ(InstanceState::IDLE, i2.state());
    EXPECT_EQ(InstanceState::IDLE, i3.state());

    std::vector<Message<std::string>> messages = i3.Prepare(std::move(value));
    ASSERT_EQ(1, messages.size());
    std::vector<Message<std::string>> promises = i2.ReceiveMessages(messages);
    ASSERT_EQ(1, promises.size());
    const Message<std::string>& promise = promises[0];
    EXPECT_EQ(MessageType::PROMISE, promise.message_type());
    const std::vector<uint32_t> expected_message_targets { 2 };
    EXPECT_EQ(expected_message_targets, promise.target_node_ids());

    std::unique_ptr<std::string> value2(new std::string("there"));
    std::vector<Message<std::string>> late_proposals = i1.Prepare(std::move(value2));
    EXPECT_EQ(InstanceRole::PROPOSER, i1.role());
    EXPECT_EQ(InstanceState::PREPARING, i1.state());
    ASSERT_EQ(1, late_proposals.size());
    const Message<std::string>& late_proposal = late_proposals[0];
    EXPECT_EQ(MessageType::PREPARE, late_proposal.message_type());
    const ProposalId expected_proposal(0, 3);
    EXPECT_EQ(expected_proposal, late_proposal.proposal_id());
    const std::vector<uint32_t> expected_node_ids {0, 1, 2};
    EXPECT_EQ(expected_node_ids, late_proposal.target_node_ids());
    EXPECT_EQ(nullptr, late_proposal.value());

    std::vector<Message<std::string>> responses = i2.ReceiveMessages(late_proposals);
    ASSERT_EQ(1, responses.size());
    const Message<std::string>& response = responses[0];
    EXPECT_EQ(MessageType::REJECT, response.message_type());
    const std::vector<uint32_t> expected_response_targets { 0 };
    EXPECT_EQ(expected_response_targets, response.target_node_ids());
    const ProposalId highest_proposal(2, 5);
    EXPECT_EQ(highest_proposal, i2.highest_promise());
}

TEST(Paxos, QuorumAcceptNullValue) {
    std::unique_ptr<std::string> value(new std::string("hello"));
    std::vector<uint32_t> current_node_ids = {0, 1, 2};
    Instance<std::string> i1(InstanceRole::ACCEPTOR, 0, 0, current_node_ids);
    Instance<std::string> i2(InstanceRole::ACCEPTOR, 0, 1, current_node_ids);
    Instance<std::string> i3(InstanceRole::ACCEPTOR, 0, 2, current_node_ids);

    auto prepare = i1.Prepare(std::move(value));
    ASSERT_EQ(1, prepare.size());
    ASSERT_EQ(InstanceState::PREPARING, i1.state());
    auto p1 = i2.ReceiveMessages(prepare);
    auto p2 = i3.ReceiveMessages(prepare);
    ASSERT_EQ(1, p1.size());
    ASSERT_EQ(1, p2.size());
    ASSERT_EQ(InstanceState::PROMISED, i2.state());
    ASSERT_EQ(InstanceState::PROMISED, i3.state());

    ASSERT_EQ(0, i1.ReceiveMessages(p1).size());
    auto accept = i1.ReceiveMessages(p2);
    ASSERT_EQ(1, accept.size());

    auto a1 = i1.ReceiveMessages(accept);
    auto a2 = i2.ReceiveMessages(accept);
    auto a3 = i3.ReceiveMessages(accept);
    ASSERT_EQ(1, a2.size());
    ASSERT_EQ(1, a3.size());

    ASSERT_EQ(InstanceState::COMPLETE, i1.state());
    ASSERT_EQ(InstanceState::COMPLETE, i2.state());
    ASSERT_EQ(InstanceState::COMPLETE, i3.state());
}
}
