#include "round.h"

namespace ledgerd {
namespace paxos {

Round::Round(unsigned int n_nodes)
    : round_n_(0),
      n_nodes_(n_nodes) { }

unsigned int Round::NextRound() {
    return round_n_++;
}

bool Round::IsQuorum() const {
    (double)promised_nodes_.size() / (double)n_nodes_ > 0.5F;
}

const std::vector<uint32_t>& Round::promised_nodes() const {
    return promised_nodes_;
}

unsigned int Round::n_nodes() const {
    return n_nodes_;
}

unsigned int Round::round_n() const {
    return round_n_;
}

}
}
