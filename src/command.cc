#include "command.h"

namespace ledgerd {
CommandType PingCommand::type() const {
    return CommandType::PING;
}

const std::string PingCommand::name() const {
    return "ping";
}

OpenTopicCommand::OpenTopicCommand(const std::string& topic_name,
                 uint32_t partition_count)
    : topic_name_(topic_name),
      partition_count_(partition_count) { }

CommandType OpenTopicCommand::type() const {
    return CommandType::OPEN_TOPIC;
}

const std::string OpenTopicCommand::name() const {
    return "open_topic";
}

const std::string& OpenTopicCommand::topic_name() const {
    return topic_name_;
}

uint32_t OpenTopicCommand::partition_count() const {
    return partition_count_;
}
}
