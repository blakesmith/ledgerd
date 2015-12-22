#include <stdexcept>
#include <sstream>

#include "cluster_log.h"

namespace ledgerd {

const std::string ClusterLog::TOPIC_NAME = "cluster_log";

ClusterLog::ClusterLog(LedgerdService& ledger_service)
    : ledger_service_(ledger_service) {
    ledger_status rc;
    ledger_topic_options options;

    ledger_topic_options_init(&options);
    const std::vector<unsigned int> partition_ids { 0 };
    rc = ledger_service_.OpenTopic(TOPIC_NAME, partition_ids, &options);
    if(rc != LEDGER_OK) {
        std::stringstream ss;
        ss << "Failed to open cluster log topic: " << rc;
        throw std::runtime_error(ss.str());
    }
}

ClusterLog::~ClusterLog() {
    // TODO: Close the topic?
}

paxos::LogStatus ClusterLog::Write(uint64_t sequence, const ClusterEvent* event) {
    ledger_status rc;
    ledger_write_status write_status;
    std::string out;

    event->SerializeToString(&out);

    rc = ledger_service_.WritePartition(TOPIC_NAME, 0, out, &write_status);
    if(rc != LEDGER_OK) {
        return paxos::LogStatus::LOG_ERR;
    }
    if(write_status.message_id != sequence - 1) {
        return paxos::LogStatus::LOG_INCONSISTENT;
    }

    return paxos::LogStatus::LOG_OK;
}

std::unique_ptr<ClusterEvent> ClusterLog::Get(uint64_t sequence) {
    ledger_status rc;
    ledger_message_set messages;

    rc = ledger_service_.ReadPartition(TOPIC_NAME, 0, sequence - 1, 1, &messages);
    if(rc != LEDGER_OK) {
        return nullptr;
    }
    if(messages.nmessages != 1) {
        return nullptr;
    }
    std::string str(static_cast<const char*>(messages.messages[0].data),
                    messages.messages[0].len);
    std::unique_ptr<ClusterEvent> event = std::unique_ptr<ClusterEvent>(
        new ClusterEvent());
    event->ParseFromString(str);
    ledger_message_set_free(&messages);

    return event;
}

uint64_t ClusterLog::HighestSequence() {
    ledger_status rc;
    uint64_t highest;

    rc = ledger_service_.LatestMessageId(TOPIC_NAME, 0, &highest);
    if(rc != LEDGER_OK) {
        return 0;
    }
    return highest;
}

}
