#ifndef LEDGERD_PAXOS_ROUND_H_
#define LEDGERD_PAXOS_ROUND_H_

#include <iostream>
#include <cassert>
#include <cstdint>
#include <vector>

namespace ledgerd {
namespace paxos {

template <typename T>
class Round {
    unsigned int round_n_;
    unsigned int n_nodes_;
    std::vector<uint32_t> promised_nodes_;
    std::pair<ProposalId, std::unique_ptr<T>> highest_promise_;
public:
    Round(unsigned int n_nodes)
        : round_n_(0),
          n_nodes_(n_nodes),
          highest_promise_(ProposalId(0, 0), nullptr) { }

    unsigned int NextRound() {
        promised_nodes_.clear();
        return round_n_++;
    }

    bool IsQuorum() const {
        return (double)promised_nodes_.size() / (double)n_nodes_ > 0.5F;
    }

    void AddPromise(uint32_t node_id)  {
        assert(promised_nodes_.size() < n_nodes_);
        promised_nodes_.push_back(node_id);
    }

    const std::vector<uint32_t>& promised_nodes() const {
        return promised_nodes_;
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
