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

    i1.set_proposed_value(std::move(value));
    std::vector<Message<std::string>> messages = i1.Prepare();
    EXPECT_EQ(InstanceRole::PROPOSER, i1.role());
    EXPECT_EQ(InstanceState::PREPARING, i1.state());
    ASSERT_EQ(1, messages.size());

    const Message<std::string>& message = messages[0];
    EXPECT_EQ(MessageType::PREPARE, message.message_type());
    const ProposalId expected_proposal(0, 1);
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
    i3.set_proposed_value(std::move(value2));
    std::vector<Message<std::string>> late_prepare = i3.Prepare();
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

    i3.set_proposed_value(std::move(value));
    std::vector<Message<std::string>> messages = i3.Prepare();
    ASSERT_EQ(1, messages.size());
    std::vector<Message<std::string>> promises = i2.ReceiveMessages(messages);
    ASSERT_EQ(1, promises.size());
    const Message<std::string>& promise = promises[0];
    EXPECT_EQ(MessageType::PROMISE, promise.message_type());
    const std::vector<uint32_t> expected_message_targets { 2 };
    EXPECT_EQ(expected_message_targets, promise.target_node_ids());

    std::unique_ptr<std::string> value2(new std::string("there"));
    i1.set_proposed_value(std::move(value2));
    std::vector<Message<std::string>> late_proposals = i1.Prepare();
    EXPECT_EQ(InstanceRole::PROPOSER, i1.role());
    EXPECT_EQ(InstanceState::PREPARING, i1.state());
    ASSERT_EQ(1, late_proposals.size());
    const Message<std::string>& late_proposal = late_proposals[0];
    EXPECT_EQ(MessageType::PREPARE, late_proposal.message_type());
    const ProposalId expected_proposal(0, 1);
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
    const ProposalId highest_proposal(2, 1);
    EXPECT_EQ(highest_proposal, i2.highest_promise());
}

TEST(Paxos, QuorumAcceptNullValue) {
    std::unique_ptr<std::string> value(new std::string("hello"));
    std::vector<uint32_t> current_node_ids = {0, 1, 2};
    Instance<std::string> i1(InstanceRole::ACCEPTOR, 0, 0, current_node_ids);
    Instance<std::string> i2(InstanceRole::ACCEPTOR, 0, 1, current_node_ids);
    Instance<std::string> i3(InstanceRole::ACCEPTOR, 0, 2, current_node_ids);

    i1.set_proposed_value(std::move(value));
    auto prepare = i1.Prepare();
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

    auto a2 = i2.ReceiveMessages(accept);
    auto a3 = i3.ReceiveMessages(accept);
    ASSERT_EQ(1, a2.size());
    ASSERT_EQ(1, a3.size());

    ASSERT_EQ(InstanceState::ACCEPTING, i1.state());
    ASSERT_EQ(InstanceState::ACCEPTING, i2.state());
    ASSERT_EQ(InstanceState::ACCEPTING, i3.state());

    ASSERT_EQ(0, i1.ReceiveMessages(a2).size());
    auto complete = i1.ReceiveMessages(a3);
    ASSERT_EQ(1, complete.size());

    ASSERT_EQ(0, i2.ReceiveMessages(complete).size());
    ASSERT_EQ(0, i3.ReceiveMessages(complete).size());

    ASSERT_EQ(InstanceState::COMPLETE, i1.state());
    ASSERT_EQ(InstanceState::COMPLETE, i2.state());
    ASSERT_EQ(InstanceState::COMPLETE, i3.state());

    ASSERT_TRUE(i1.final_value() != nullptr);
    ASSERT_TRUE(i2.final_value() != nullptr);
    ASSERT_TRUE(i3.final_value() != nullptr);
    EXPECT_EQ("hello", *i1.final_value());
    EXPECT_EQ("hello", *i2.final_value());
    EXPECT_EQ("hello", *i3.final_value());
}

TEST(Paxos, QuorumExistingAcceptedValue) {
    std::unique_ptr<std::string> value(new std::string("hello"));
    std::vector<uint32_t> current_node_ids = {0, 1, 2, 3, 4};
    Instance<std::string> i1(InstanceRole::ACCEPTOR, 0, 0, current_node_ids);
    Instance<std::string> i2(InstanceRole::ACCEPTOR, 0, 1, current_node_ids);
    Instance<std::string> i3(InstanceRole::ACCEPTOR, 0, 2, current_node_ids);
    Instance<std::string> i4(InstanceRole::ACCEPTOR, 0, 3, current_node_ids);
    Instance<std::string> i5(InstanceRole::ACCEPTOR, 0, 4, current_node_ids);

    i1.set_proposed_value(std::move(value));
    auto prepare = i1.Prepare();
    ASSERT_EQ(1, prepare.size());
    ASSERT_EQ(InstanceState::PREPARING, i1.state());
    auto p1 = i2.ReceiveMessages(prepare);
    auto p2 = i3.ReceiveMessages(prepare);
    auto p3 = i4.ReceiveMessages(prepare);
    auto p4 = i5.ReceiveMessages(prepare);
    ASSERT_EQ(1, p1.size());
    ASSERT_EQ(1, p2.size());
    ASSERT_EQ(1, p3.size());
    ASSERT_EQ(InstanceState::PROMISED, i2.state());
    ASSERT_EQ(InstanceState::PROMISED, i3.state());
    ASSERT_EQ(InstanceState::PROMISED, i4.state());

    ASSERT_EQ(0, i1.ReceiveMessages(p1).size());
    ASSERT_EQ(0, i1.ReceiveMessages(p2).size());
    auto accept = i1.ReceiveMessages(p3);
    ASSERT_EQ(1, accept.size());
    const Message<std::string>& accept_message = accept[0];
    const std::vector<uint32_t> expected_node_ids { 1, 2, 3 };
    EXPECT_EQ(expected_node_ids, accept_message.target_node_ids());
    auto accept2 = i1.ReceiveMessages(p4);
    ASSERT_EQ(1, accept2.size());
    const Message<std::string>& accept_message2 = accept2[0];
    const std::vector<uint32_t> expected_node_ids2 { 4 };
    EXPECT_EQ(expected_node_ids2, accept_message2.target_node_ids());

    auto a2 = i2.ReceiveMessages(accept);
    auto a3 = i3.ReceiveMessages(accept);
    auto a4 = i4.ReceiveMessages(accept);
    ASSERT_EQ(1, a2.size());
    ASSERT_EQ(1, a3.size());
    ASSERT_EQ(1, a4.size());

    ASSERT_EQ(InstanceState::ACCEPTING, i1.state());
    ASSERT_EQ(InstanceState::ACCEPTING, i2.state());
    ASSERT_EQ(InstanceState::ACCEPTING, i3.state());

    ASSERT_EQ(0, i1.ReceiveMessages(a2).size());
    ASSERT_EQ(0, i1.ReceiveMessages(a3).size());
    auto complete = i1.ReceiveMessages(a4);
    ASSERT_EQ(1, complete.size());
    const Message<std::string>& complete_message = complete[0];
    EXPECT_EQ(expected_node_ids, complete_message.target_node_ids());    

    ASSERT_EQ(0, i2.ReceiveMessages(complete).size());
    ASSERT_EQ(0, i3.ReceiveMessages(complete).size());
    ASSERT_EQ(0, i4.ReceiveMessages(complete).size());

    ASSERT_EQ(InstanceState::COMPLETE, i1.state());
    ASSERT_EQ(InstanceState::COMPLETE, i2.state());
    ASSERT_EQ(InstanceState::COMPLETE, i3.state());
    ASSERT_EQ(InstanceState::COMPLETE, i4.state());

    ASSERT_TRUE(i1.final_value() != nullptr);
    ASSERT_TRUE(i2.final_value() != nullptr);
    ASSERT_TRUE(i3.final_value() != nullptr);
    EXPECT_EQ("hello", *i1.final_value());
    EXPECT_EQ("hello", *i2.final_value());
    EXPECT_EQ("hello", *i3.final_value());

    std::unique_ptr<std::string> second_value(new std::string("there"));
    i5.set_proposed_value(std::move(second_value));
    auto second_prepare = i5.Prepare();

    auto tp1 = i1.ReceiveMessages(second_prepare);
    auto tp2 = i2.ReceiveMessages(second_prepare);
    auto tp3 = i3.ReceiveMessages(second_prepare);
    auto tp4 = i4.ReceiveMessages(second_prepare);

    ASSERT_EQ(1, tp1.size());
    ASSERT_EQ(1, tp2.size());
    ASSERT_EQ(1, tp3.size());
    ASSERT_EQ(1, tp4.size());

    EXPECT_EQ("hello", *tp1[0].value());
    EXPECT_EQ("hello", *tp2[0].value());
    EXPECT_EQ("hello", *tp3[0].value());
    EXPECT_EQ("hello", *tp4[0].value());

    ASSERT_EQ(0, i5.ReceiveMessages(tp1).size());
    ASSERT_EQ(0, i5.ReceiveMessages(tp2).size());
    auto second_accept_1 = i5.ReceiveMessages(tp3);
    ASSERT_EQ(1, second_accept_1.size());
    auto second_accept_2 = i5.ReceiveMessages(tp4);
    ASSERT_EQ(1, second_accept_2.size());

    i5.ReceiveMessages(i1.ReceiveMessages(second_accept_1));
    i5.ReceiveMessages(i1.ReceiveMessages(second_accept_2));
    i5.ReceiveMessages(i2.ReceiveMessages(second_accept_1));
    i5.ReceiveMessages(i2.ReceiveMessages(second_accept_2));
    i5.ReceiveMessages(i3.ReceiveMessages(second_accept_1));
    i5.ReceiveMessages(i3.ReceiveMessages(second_accept_2));
    i5.ReceiveMessages(i4.ReceiveMessages(second_accept_1));
    i5.ReceiveMessages(i4.ReceiveMessages(second_accept_2));

    ASSERT_TRUE(i1.final_value() != nullptr);
    ASSERT_TRUE(i2.final_value() != nullptr);
    ASSERT_TRUE(i3.final_value() != nullptr);
    ASSERT_TRUE(i4.final_value() != nullptr);
    ASSERT_TRUE(i5.final_value() != nullptr);
    EXPECT_EQ("hello", *i1.final_value());
    EXPECT_EQ("hello", *i2.final_value());
    EXPECT_EQ("hello", *i3.final_value());
    EXPECT_EQ("hello", *i4.final_value());
    EXPECT_EQ("hello", *i5.final_value());
}

TEST(Paxos, ExistingValueRejectMultiRound) {
    std::unique_ptr<std::string> value(new std::string("hello"));
    std::vector<uint32_t> current_node_ids = {0, 1, 2, 3, 4};
    Instance<std::string> i1(InstanceRole::ACCEPTOR, 0, 0, current_node_ids);
    Instance<std::string> i2(InstanceRole::ACCEPTOR, 0, 1, current_node_ids);
    Instance<std::string> i3(InstanceRole::ACCEPTOR, 0, 2, current_node_ids);
    Instance<std::string> i4(InstanceRole::ACCEPTOR, 0, 3, current_node_ids);
    Instance<std::string> i5(InstanceRole::ACCEPTOR, 0, 4, current_node_ids);

    i5.set_proposed_value(std::move(value));
    auto prepare = i5.Prepare();
    i5.ReceiveMessages(i2.ReceiveMessages(prepare));
    i5.ReceiveMessages(i3.ReceiveMessages(prepare));
    auto accept = i5.ReceiveMessages(i4.ReceiveMessages(prepare));

    i5.ReceiveMessages(i2.ReceiveMessages(accept));
    i5.ReceiveMessages(i3.ReceiveMessages(accept));
    auto decided = i5.ReceiveMessages(i4.ReceiveMessages(accept));

    i2.ReceiveMessages(decided);
    i3.ReceiveMessages(decided);
    i4.ReceiveMessages(decided);

    ASSERT_TRUE(i2.final_value() != nullptr);
    ASSERT_TRUE(i3.final_value() != nullptr);
    ASSERT_TRUE(i4.final_value() != nullptr);
    ASSERT_TRUE(i5.final_value() != nullptr);
    EXPECT_EQ("hello", *i2.final_value());
    EXPECT_EQ("hello", *i3.final_value());
    EXPECT_EQ("hello", *i4.final_value());
    EXPECT_EQ("hello", *i5.final_value());

    std::unique_ptr<std::string> value_2(new std::string("there"));
    i1.set_proposed_value(std::move(value_2));
    auto p2 = i1.Prepare();
    auto rejects = i2.ReceiveMessages(p2);
    ASSERT_EQ(1, rejects.size());
    const Message<std::string>& reject = rejects[0];
    EXPECT_EQ(MessageType::REJECT, reject.message_type());

    auto p3 = i1.Prepare();
    i1.ReceiveMessages(i2.ReceiveMessages(p3));
    i1.ReceiveMessages(i3.ReceiveMessages(p3));
    i1.ReceiveMessages(i4.ReceiveMessages(p3));
    auto accept2 = i1.ReceiveMessages(i5.ReceiveMessages(p3));

    i1.ReceiveMessages(i2.ReceiveMessages(accept2));
    i1.ReceiveMessages(i3.ReceiveMessages(accept2));
    i1.ReceiveMessages(i4.ReceiveMessages(accept2));
    auto decided2 = i1.ReceiveMessages(i5.ReceiveMessages(accept2));

    i2.ReceiveMessages(decided2);
    i3.ReceiveMessages(decided2);
    i4.ReceiveMessages(decided2);
    i5.ReceiveMessages(decided2);

    ASSERT_TRUE(i1.final_value() != nullptr);
    ASSERT_TRUE(i2.final_value() != nullptr);
    ASSERT_TRUE(i3.final_value() != nullptr);
    ASSERT_TRUE(i4.final_value() != nullptr);
    ASSERT_TRUE(i5.final_value() != nullptr);
    EXPECT_EQ("hello", *i1.final_value());
    EXPECT_EQ("hello", *i2.final_value());
    EXPECT_EQ("hello", *i3.final_value());
    EXPECT_EQ("hello", *i4.final_value());
    EXPECT_EQ("hello", *i5.final_value());
}

TEST(Paxos, LeapFroggingProposers) {
}
}
