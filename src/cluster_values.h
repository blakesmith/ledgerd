#ifndef LEDGERD_CLUSTER_VALUES_H_
#define LEDGERD_CLUSTER_VALUES_H_

#include <vector>

namespace ledgerd {

struct ClusterTopic {
    std::string name;
    std::vector<uint32_t> partition_ids;
};

struct ClusterTopicList {
    std::vector<ClusterTopic> topics;
};

enum class ClusterValueType {
    TOPIC_LIST
};

struct ClusterValue {
    ClusterValueType type;
    ClusterTopicList topic_list;
};

}

#endif
