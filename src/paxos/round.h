#ifndef LEDGERD_PAXOS_ROUND_H_
#define LEDGERD_PAXOS_ROUND_H_

#include <cstdint>
#include <vector>

namespace ledgerd {
namespace paxos {

class Round {
    unsigned int round_n_;
    unsigned int n_nodes_;
    std::vector<uint32_t> promised_nodes_;
public:
    Round(unsigned int n_nodes);
    unsigned int NextRound();
    bool IsQuorum() const;
    void AddPromise(uint32_t node_id);
    const std::vector<uint32_t>& promised_nodes() const;
    unsigned int n_nodes() const;
    unsigned int round_n() const;
};

}
}

#endif
