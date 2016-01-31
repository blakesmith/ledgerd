#ifndef LEDGERD_CLUSTER_LISTENER_H_
#define LEDGERD_CLUSTER_LISTENER_H_

#include <map>

#include "cluster_values.h"
#include "paxos/listener.h"
#include "proto/ledgerd.pb.h"

namespace ledgerd {

class ClusterListener : public paxos::Listener<ClusterEvent, ClusterValue> {
    ClusterTopicList topic_list_;
    uint64_t highest_sequence_;
public:
    ClusterListener();
    ~ClusterListener() = default;

    virtual paxos::ListenerStatus Receive(uint64_t sequence, const ClusterEvent* event);
    virtual paxos::ListenerStatus Map(const ClusterEvent* event, ClusterValue* out);
    virtual uint64_t HighestSequence();
};
}

#endif
