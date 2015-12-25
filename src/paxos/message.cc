#include "message.h"

namespace ledgerd {
namespace paxos {

ProposalId::ProposalId(uint32_t node_id, uint32_t prop_n)
    : node_id_(node_id), prop_n_(prop_n) { }

int ProposalId::compare(const ProposalId& rhs) const {
    if(this->prop_n_ > rhs.prop_n_) {
        return -1;
    } else if(this->prop_n_ < rhs.prop_n_) {
        return 1;
    } else {
        if(this->node_id_ > rhs.node_id_) {
            return -1;
        } else if(this->node_id_ < rhs.node_id_) {
            return 1;
        }
    }

    return 0;
}

bool ProposalId::operator==(const ProposalId& rhs) const {
    return compare(rhs) == 0;
}

bool ProposalId::operator!=(const ProposalId& rhs) const {
    return compare(rhs) != 0;
}

bool ProposalId::operator<(const ProposalId& rhs) const {
    return compare(rhs) == 1;
}

bool ProposalId::operator<=(const ProposalId& rhs) const {
    return compare(rhs) == 1 || compare(rhs) == 0;
}

bool ProposalId::operator>(const ProposalId& rhs) const {
    return compare(rhs) == -1;
}

bool ProposalId::operator>=(const ProposalId& rhs) const {
    return compare(rhs) == -1 || compare(rhs) == 0;
}

uint32_t ProposalId::node_id() const {
    return node_id_;
}

uint32_t ProposalId::prop_n() const {
    return prop_n_;
}

ProposalId& ProposalId::operator=(const ProposalId& rhs) {
    node_id_ = rhs.node_id_;
    prop_n_ = rhs.prop_n_;
    return *this;
}

ProposalId& ProposalId::operator=(ProposalId&& rhs) {
    std::swap(node_id_, rhs.node_id_);
    std::swap(prop_n_, rhs.prop_n_);
    return *this;
}

}
}
