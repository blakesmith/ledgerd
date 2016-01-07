#ifndef LEDGERD_PAXOS_LINEAR_SEQUENCE_H_
#define LEDGERD_PAXOS_LINEAR_SEQUENCE_H_

#include <condition_variable>
#include <map>
#include <mutex>
#include <set>

namespace ledgerd {
namespace paxos {

template <typename T,
          typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>

struct WaitGroup {
    std::mutex lock;
    std::condition_variable cond;
    T value;

    WaitGroup(T v)
        : value(v) { }

    bool operator<(WaitGroup<T> rhs) const {
        return value > rhs.value;
    }
};

template <typename T,
          typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>

class LinearSequence {
    T lower_bound_;
    T upper_bound_;
    std::set<T> disjoint_values_;
    std::mutex wait_mutex_;
    std::map<T, std::unique_ptr<WaitGroup<T>>> waiters_;

    void notify_waiters(T n) {
        std::lock_guard<std::mutex> lk(wait_mutex_);
        auto search = waiters_.find(n);
        if(search != waiters_.end()) {
            std::lock_guard<std::mutex> lk(search->second->lock);
            search->second->cond.notify_all();
            waiters_.erase(n);
        }
    }
public:
    LinearSequence(T lower_bound)
        : lower_bound_(lower_bound),
          upper_bound_(lower_bound) { }

    void Add(T n) {
        if(n == upper_bound_ + 1) {
            upper_bound_ = n;
            notify_waiters(upper_bound_);
            for(auto it = disjoint_values_.begin(); it != disjoint_values_.end(); ++it) {
                if(*it == upper_bound_ + 1) {
                    upper_bound_ = *it;
                    disjoint_values_.erase(it);
                    notify_waiters(upper_bound_);
                } else {
                    break;
                }
            }
        } else {
            disjoint_values_.insert(n);
        }
    }

    void WaitFor(T n) {
        wait_mutex_.lock();
        auto search = waiters_.find(n);
        WaitGroup<T>* group_ref;
        if(search == waiters_.end()) {
            std::unique_ptr<WaitGroup<T>> new_group(
                new WaitGroup<T>(n));
            group_ref = new_group.get();
            waiters_[n] = std::move(new_group);
        } else {
            group_ref = search->second.get();
        }
        wait_mutex_.unlock();
        std::unique_lock<std::mutex> glk(group_ref->lock);
        group_ref->cond.wait(glk, [this, n] { return upper_bound() >= n; });
    }

    T next() const{
        return upper_bound_ + 1;
    }

    T upper_bound() const {
        return upper_bound_;
    }

    T lower_bound() const {
        return lower_bound_;
    }

    void set_upper_bound(T v) {
        upper_bound_ = v;
    }

    unsigned int n_disjoint() const {
        return disjoint_values_.size();
    }

    bool in_joint_range(T v) {
        return lower_bound_ <= v && upper_bound_ >= v;
    }
};

}
}

#endif
