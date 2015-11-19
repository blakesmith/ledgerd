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
    ProposalId(uint32_t node_id, uint32_t prop_n);
    bool operator==(const ProposalId& rhs) const;
    bool operator!=(const ProposalId& rhs) const;
    bool operator<(const ProposalId& rhs) const;
    bool operator<=(const ProposalId& rhs) const;
    bool operator>(const ProposalId& rhs) const;
    bool operator>=(const ProposalId& rhs) const;
    uint32_t node_id() const;
    uint32_t prop_n() const;
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
};

}
}

#endif
