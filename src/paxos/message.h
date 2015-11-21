#ifndef LEDGERD_PAXOS_MESSAGE_H_
#define LEDGERD_PAXOS_MESSAGE_H_

#include <cstdint>
#include <memory>

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
    ProposalId& operator=(ProposalId&& rhs) = delete;
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
    const ProposalId proposal_id_;
    const MessageType message_type_;
    bool value_message_;
    std::unique_ptr<T> value_;
public:
    Message(const MessageType& message_type,
            const ProposalId& proposal_id,
            std::unique_ptr<T> value)
        : message_type_(message_type),
          proposal_id_(proposal_id),
          value_(std::move(value)) {
        switch (message_type) {
            case PREPARE:
            case REJECT:
                value_message_(false);
                break;
            case PROMISE:
            case ACCEPT:
            case ACCEPTED:
                value_message_(true);
                break;
            default:
                value_message_(false);
                break;
        }
    }

    Message(const MessageType& message_type,
            const ProposalId& proposal_id)
        : Message(message_type, proposal_id, std::unique_ptr<T>(nullptr)) { }

    Message(const Message& rhs)
        : proposal_id_(rhs.proposal_id_),
          message_type_(rhs.message_type_),
          value_message_(rhs.value_message_),
          value_(new T(*rhs.value_)) { }

    Message(Message&& rhs) = default;
    ~Message() = default;

    const MessageType& message_type() const {
        return message_type_;
    }

    bool value_message() const {
        return value_message_;
    }

    T* value() const {
        return value_.get();
    }

    const ProposalId& proposal_id() const {
        return proposal_id_;
    }

    Message& operator=(const Message& rhs) {
        proposal_id_ = rhs.proposal_id_;
        message_type_ = rhs.message_type_;
        value_message_ = rhs.value_message_;
        value_ = std::unique_ptr<T>(new T(*rhs.value_));
        return *this;
    }

    Message& operator=(Message&& rhs) {
        std::swap(proposal_id_, rhs.proposal_id_);
        std::swap(message_type_, rhs.message_type_);
        std::swap(value_message_, rhs.value_message_);
        std::swap(value_, rhs.value_);
        return *this;
    }
};

}
}

#endif
