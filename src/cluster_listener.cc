#include "cluster_listener.h"

namespace ledgerd {

ClusterListener::ClusterListener()
    : highest_sequence_(0) { }

paxos::ListenerStatus ClusterListener::Receive(uint64_t sequence, const ClusterEvent* event) {
    switch(event->type()) {
        case ClusterEventType::REGISTER_TOPIC: {
            const RegisterTopicEvent& register_topic = event->register_topic();
            ClusterTopic topic;
            topic.name = register_topic.name();
            for(int i = 0; i < register_topic.partition_ids_size(); i++) {
                topic.partition_ids.push_back(register_topic.partition_ids(i));
            }
            topic_list_.topics.push_back(std::move(topic));
            break;
        }
        default:
            break;
    }
    highest_sequence_ = sequence;
    return paxos::ListenerStatus::OK;
}

paxos::ListenerStatus ClusterListener::Map(const ClusterEvent* event, ClusterValue* cluster_value) {
    switch(event->type()) {
        case ClusterEventType::LIST_TOPICS: {
            cluster_value->type = ClusterValueType::TOPIC_LIST;
            cluster_value->topic_list = topic_list_;
            break;
        }
        default:
            break;
    }
    return paxos::ListenerStatus::OK;
}

uint64_t ClusterListener::HighestSequence() {
    return highest_sequence_;
}

}
