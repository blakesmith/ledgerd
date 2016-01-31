#include "cluster_listener.h"

namespace ledgerd {

ClusterListener::ClusterListener()
    : highest_sequence_(0) { }

paxos::ListenerStatus ClusterListener::Receive(uint64_t sequence, const ClusterEvent* event) {
    return paxos::ListenerStatus::OK;
}

paxos::ListenerStatus ClusterListener::Map(const ClusterEvent* event, ClusterValue* out) {
    return paxos::ListenerStatus::OK;
}

uint64_t ClusterListener::HighestSequence() {
    return highest_sequence_;
}

}
