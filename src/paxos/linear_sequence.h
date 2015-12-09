#ifndef LEDGERD_PAXOS_LINEAR_SEQUENCE_H_
#define LEDGERD_PAXOS_LINEAR_SEQUENCE_H_

#include <set>

namespace ledgerd {
namespace paxos {

template <typename T,
          typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>

class LinearSequence {
    T lower_bound_;
    T upper_bound_;
    std::set<T> disjoint_values_;
public:
    LinearSequence(T lower_bound)
        : lower_bound_(lower_bound),
          upper_bound_(lower_bound) { }

    void Add(T n) {
        if(n == upper_bound_ + 1) {
            upper_bound_ = n;
            for(auto it = disjoint_values_.begin(); it != disjoint_values_.end(); ++it) {
                if(*it == upper_bound_ + 1) {
                    upper_bound_ = *it;
                    disjoint_values_.erase(it);
                } else {
                    break;
                }
            }
        } else {
            disjoint_values_.insert(n);
        }
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

    unsigned int n_disjoint() const {
        return disjoint_values_.size();
    }
};

}
}

#endif
