#ifndef LEDGERD_PAXOS_EVENT_H_
#define LEDGERD_PAXOS_EVENT_H_

#include <vector>

#include "message.h"

namespace ledgerd {
namespace paxos {

template <typename T>
class Event {
    std::vector<Message<T>> messages_;
    const T* final_value_;

public:
    Event()
        : messages_(std::vector<Message<T>>{}),
          final_value_(nullptr) { }

    Event(std::vector<Message<T>>&& messages)
        : messages_(std::move(messages)),
          final_value_(nullptr) { }

    Event(const T* final_value)
        : messages_(std::vector<Message<T>>{}),
          final_value_(final_value) { }

    Event(const Event& rhs)
        : messages_(rhs.messages_),
          final_value_(rhs.final_value_) { }

    Event(Event&& rhs) = default;
    ~Event() = default;

    bool HasMessages() const {
        return messages_.size() > 0;
    }

    bool HasFinalValue() const {
        return final_value_ != nullptr;
    }

    const std::vector<Message<T>>& messages() const {
        return messages_;
    }

    const T* final_value() const {
        return final_value_;
    }

    Event& operator=(const Event& rhs) {
        messages_ = rhs.messages_;
        final_value_ = rhs.final_value_;
        return *this;
    }

    Event& operator=(Event&& rhs) {
        std::swap(messages_, rhs.messages_);
        std::swap(final_value_, rhs.final_value_);
        return *this;
    }
};
}
}

#endif
