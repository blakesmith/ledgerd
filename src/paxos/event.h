#ifndef LEDGERD_PAXOS_EVENT_H_
#define LEDGERD_PAXOS_EVENT_H_

#include <vector>

#include "message.h"

namespace ledgerd {
namespace paxos {

template <typename T>
class Event {
    std::vector<Message<T>> messages_;
    std::unique_ptr<T> value_;

    std::unique_ptr<T> copy_unique(const std::unique_ptr<T>& source) {
        return source ? std::unique_ptr<T>(new T(*source)) : nullptr;
    }
public:
    Event()
        : messages_(std::vector<Message<T>>{}),
          value_(nullptr) { }

    Event(std::vector<Message<T>>&& messages)
        : messages_(std::move(messages)),
          value_(nullptr) { }

    Event(std::unique_ptr<T> value)
        : messages_(std::vector<Message<T>>{}),
          value_(std::move(value)) { }

    Event(const Event& rhs)
        : messages_(rhs.messages_),
          value_(copy_unique(rhs.value_)) { }

    Event(Event&& rhs) = default;
    ~Event() = default;

    bool HasMessages() const {
        return messages_.size() > 0;
    }

    bool HasValue() const {
        return value_ != nullptr;
    }

    const std::vector<Message<T>>& messages() const {
        return messages_;
    }

    const T& value() const {
        return *value_;
    }

    Event& operator=(const Event& rhs) {
        messages_ = rhs.messages;
        value_ = copy_unique(rhs.value_);
        return *this;
    }

    Event& operator=(Event&& rhs) {
        std::swap(messages_, rhs.messages_);
        std::swap(value_, rhs.value);
        return *this;
    }
};
}
}

#endif
