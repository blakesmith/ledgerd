#ifndef LEDGERD_PAXOS_VALUE_H_
#define LEDGERD_PAXOS_VALUE_H_

#include <memory>

namespace ledgerd {
namespace paxos {

template <typename T>
class Value {
    uint64_t id_;
    std::unique_ptr<T> value_;

    void copy_value(std::unique_ptr<T>& dst,
                    const std::unique_ptr<T>& src) {
        if(src != nullptr) {
            dst = std::unique_ptr<T>(new T(*src));
        } else {
            dst = nullptr;
        }
    }

public:
    Value(uint64_t id,
          std::unique_ptr<T> value)
        : id_(id),
          value_(std::move(value)) { }

    ~Value() = default;

    Value(const Value& rhs) 
        : id_(rhs.id_) {
        copy_value(value_, rhs.value_);
    }

    Value(Value&& rhs) = default;

    Value& operator=(const Value& rhs) {
        id_ = rhs.id_;
        copy_value(value_, rhs.value_);
        return *this;
    }

    Value& operator=(Value&& rhs) {
        std::swap(id_, rhs.id_);
        std::swap(value_, rhs.value_);
        return *this;
    }

    uint64_t id() const {
        return id_;
    }

    const T* value() const {
        return value_.get();
    }
};

}
}

#endif
