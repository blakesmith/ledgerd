#ifndef LEDGERD_PAXOS_MESSAGE_H_
#define LEDGERD_PAXOS_MESSAGE_H_

#include <cstdint>
#include <memory>
#include <vector>

namespace ledgerd {
namespace paxos {

enum MessageType {
    PREPARE,
    PROMISE,
    REJECT,
    ACCEPT,
    ACCEPTED
};

enum AdminMessageType {
    JOIN,
    LEAVE
};

class ProposalId {
    uint32_t node_id_;
    uint32_t prop_n_;
    int compare(const ProposalId& rhs) const;
public:
    ProposalId(const ProposalId& rhs) = default;
    ProposalId(ProposalId&& rhs) = default;
    ~ProposalId() = default;
    ProposalId(uint32_t node_id, uint32_t prop_n);
    bool operator==(const ProposalId& rhs) const;
    bool operator!=(const ProposalId& rhs) const;
    bool operator<(const ProposalId& rhs) const;
    bool operator<=(const ProposalId& rhs) const;
    bool operator>(const ProposalId& rhs) const;
    bool operator>=(const ProposalId& rhs) const;
    uint32_t node_id() const;
    uint32_t prop_n() const;
    ProposalId& operator=(const ProposalId& rhs);
    ProposalId& operator=(ProposalId&& rhs);
};

class AdminMessage {
    uint32_t node_id_;
    AdminMessageType message_type_;
public:
    AdminMessage(uint32_t node_id,
                 AdminMessageType message_type);
    
    uint32_t node_id() const;
    AdminMessageType message_type() const;
};

template <typename T>
class Message {
    const MessageType message_type_;
    const ProposalId proposal_id_;
    uint32_t sequence_;
    uint32_t source_node_id_;
    const std::vector<uint32_t> target_node_ids_;
    const T* value_;
    bool value_message_;

public:
    Message(const MessageType& message_type,
            uint32_t sequence,
            const ProposalId& proposal_id,
            uint32_t source_node_id,
            const std::vector<uint32_t> target_node_ids,
            const T* value)
        : message_type_(message_type),
          sequence_(sequence),
          proposal_id_(proposal_id),
          source_node_id_(source_node_id),
          target_node_ids_(target_node_ids),
          value_(value) {
        switch (message_type) {
            case PREPARE:
            case REJECT:
                value_message_ = false;
                break;
            case PROMISE:
            case ACCEPT:
            case ACCEPTED:
                value_message_ = true;
                break;
            default:
                value_message_ = false;
                break;
        }
    }

    Message(const MessageType& message_type,
            uint32_t sequence,
            const ProposalId& proposal_id,
            uint32_t source_node_id,
            const std::vector<uint32_t> target_node_ids)
        : Message(message_type,
                  sequence,
                  proposal_id,
                  source_node_id,
                  target_node_ids,
                  nullptr) { }

    Message(const Message& rhs)
        : message_type_(rhs.message_type_),
          sequence_(rhs.sequence_),
          proposal_id_(rhs.proposal_id_),
          source_node_id_(rhs.source_node_id_),
          target_node_ids_(rhs.target_node_ids_),
          value_message_(rhs.value_message_),
          value_(rhs.value_) { }

    Message(Message&& rhs) = default;
    ~Message() = default;

    const MessageType& message_type() const {
        return message_type_;
    }

    bool value_message() const {
        return value_message_;
    }

    const T* value() const {
        return value_;
    }

    uint32_t sequence() const {
        return sequence_;
    }

    const ProposalId& proposal_id() const {
        return proposal_id_;
    }

    uint32_t source_node_id() const {
        return source_node_id_;
    }

    const std::vector<uint32_t>& target_node_ids() const {
        return target_node_ids_;
    }

    Message& operator=(const Message& rhs) {
        message_type_ = rhs.message_type_;
        sequence_ = rhs.sequence_;
        proposal_id_ = rhs.proposal_id_;
        source_node_id_ = rhs.source_node_id_;
        target_node_ids_ = rhs.target_node_ids_;
        value_message_ = rhs.value_message_;
        value_ = rhs.value_;
        return *this;
    }

    Message& operator=(Message&& rhs) {
        std::swap(message_type_, rhs.message_type_);
        std::swap(sequence_, rhs.sequence);
        std::swap(proposal_id_, rhs.proposal_id_);
        std::swap(source_node_id_, rhs.source_node_id_);
        std::swap(target_node_ids_, rhs.target_node_ids_);
        std::swap(value_message_, rhs.value_message_);
        std::swap(value_, rhs.value_);
        return *this;
    }
};

}
}

#endif
