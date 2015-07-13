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

WritePartitionCommand::WritePartitionCommand(const std::string& topic_name,
                                        uint32_t partition_num,
                                        const std::string& data)
    : topic_name_(topic_name),
      partition_num_(partition_num),
      data_(data) { }

CommandType WritePartitionCommand::type() const {
    return CommandType::WRITE_PARTITION;
}

const std::string WritePartitionCommand::name() const {
    return "write_partition";
}

const std::string& WritePartitionCommand::topic_name() const {
    return topic_name_;
}

uint32_t WritePartitionCommand::partition_num() const {
    return partition_num_;
}

const std::string& WritePartitionCommand::data() const {
    return data_;
}

ReadPartitionCommand::ReadPartitionCommand(const std::string& topic_name,
                                           uint32_t partition_num,
                                           uint64_t start_id,
                                           uint32_t nmessages)
    : topic_name_(topic_name),
      partition_num_(partition_num),
      start_id_(start_id),
      nmessages_(nmessages) { }

CommandType ReadPartitionCommand::type() const {
    return CommandType::READ_PARTITION;
}

const std::string ReadPartitionCommand::name() const {
    return "read_partition";
}

const std::string& ReadPartitionCommand::topic_name() const {
    return topic_name_;
}

uint32_t ReadPartitionCommand::partition_num() const {
    return partition_num_;
}

uint64_t ReadPartitionCommand::start_id() const {
    return start_id_;
}

uint32_t ReadPartitionCommand::nmessages() const {
    return nmessages_;
}
}
