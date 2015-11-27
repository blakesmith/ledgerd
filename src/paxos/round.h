#ifndef LEDGERD_PAXOS_ROUND_H_
#define LEDGERD_PAXOS_ROUND_H_

#include <algorithm>
#include <iostream>
#include <cassert>
#include <cstdint>
#include <set>

namespace ledgerd {
namespace paxos {

template <typename T>
class Round {
    unsigned int round_n_;
    unsigned int n_nodes_;
    std::set<uint32_t> promised_nodes_;
    std::set<uint32_t> sent_accept_nodes_;
    std::set<uint32_t> accepted_nodes_;
    std::set<uint32_t> sent_decided_nodes_;
    std::pair<ProposalId, std::unique_ptr<T>> highest_promise_;
public:
    Round(unsigned int n_nodes)
        : round_n_(0),
          n_nodes_(n_nodes),
          highest_promise_(ProposalId(0, 0), nullptr) { }

    unsigned int NextRound() {
        promised_nodes_.clear();
        accepted_nodes_.clear();
        highest_promise_ = std::make_pair(ProposalId(0, 0), nullptr);
        return round_n_++;
    }

    bool IsPromiseQuorum() const {
        return (double)promised_nodes_.size() / (double)n_nodes_ > 0.5F;
    }

    bool IsAcceptQuorum() const {
        return (double)accepted_nodes_.size() / (double)n_nodes_ > 0.5F;
    }

    void AddPromise(uint32_t node_id,
                    const ProposalId prop,
                    const T* value)  {
        assert(promised_nodes_.size() < n_nodes_);
        const ProposalId& current_highest = highest_promise_.first;
        if(prop > current_highest) {
            highest_promise_.first = prop;
            if(value) {
                highest_promise_.second = std::unique_ptr<T>(new T(*value));
            } else {
                highest_promise_.second = nullptr;
            }
        }
        promised_nodes_.insert(node_id);
    }

    void AddAccepted(uint32_t node_id,
                     const ProposalId prop,
                     const T* value) {
        assert(accepted_nodes_.size() < n_nodes_);
        accepted_nodes_.insert(node_id);
    }

    std::vector<uint32_t> TargetAcceptNodes() {
        std::vector<uint32_t> targets;
        std::set_difference(promised_nodes_.begin(), promised_nodes_.end(),
                            sent_accept_nodes_.begin(), sent_accept_nodes_.end(),
                            std::inserter(targets, targets.end()));
        for(auto& id : targets) {
            sent_accept_nodes_.insert(id);
        }
        return targets;
    }

    std::vector<uint32_t> TargetDecidedNodes() {
        std::vector<uint32_t> targets;
        std::set_difference(accepted_nodes_.begin(), accepted_nodes_.end(),
                            sent_decided_nodes_.begin(), sent_decided_nodes_.end(),
                            std::inserter(targets, targets.end()));
        for(auto& id : targets) {
            sent_decided_nodes_.insert(id);
        }
        return targets;
    }

    const T* highest_value() const {
        return highest_promise_.second.get();
    }

    unsigned int n_nodes() const {
        return n_nodes_;
    }

    unsigned int round_n() const {
        return round_n_;
    }
};

}
}

#endif
