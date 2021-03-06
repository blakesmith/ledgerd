#include "command.h"

namespace ledgerd {

Command::Command(const CommonOptions& common_opts)
    : common_opts_(common_opts) { }

const CommonOptions& Command::common_opts() const {
    return common_opts_;
}

PingCommand::PingCommand(const CommonOptions& common_opts)
    : Command(common_opts) { }

CommandType PingCommand::type() const {
    return CommandType::PING;
}

const std::string PingCommand::name() const {
    return "ping";
}

UnknownCommand::UnknownCommand(const CommonOptions& common_opts,
                               const std::string& command_name)
    : Command(common_opts),
      command_name_(command_name) { }

CommandType UnknownCommand::type() const {
    return CommandType::UNKNOWN;
}

const std::string UnknownCommand::name() const {
    return "unknown";
}

const std::string& UnknownCommand::command_name() const {
    return command_name_;
}

OpenTopicCommand::OpenTopicCommand(const CommonOptions& common_opts,
                                   const std::string& topic_name,
                                   uint32_t partition_count)
    : Command(common_opts),
      topic_name_(topic_name),
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

GetTopicCommand::GetTopicCommand(const CommonOptions& common_opts,
                                 const std::string& topic_name)
    : Command(common_opts),
      topic_name_(topic_name) { }

CommandType GetTopicCommand::type() const {
    return CommandType::GET_TOPIC;
}

const std::string& GetTopicCommand::topic_name() const {
    return topic_name_;
}

const std::string GetTopicCommand::name() const {
    return "get_topic";
}

WritePartitionCommand::WritePartitionCommand(const CommonOptions& common_opts,
                                             const std::string& topic_name,
                                             uint32_t partition_num,
                                             const std::string& data)
    : Command(common_opts),
      topic_name_(topic_name),
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

ReadPartitionCommand::ReadPartitionCommand(const CommonOptions& common_opts,
                                           const std::string& topic_name,
                                           uint32_t partition_num,
                                           uint64_t start_id,
                                           uint32_t nmessages)
    : Command(common_opts),
      topic_name_(topic_name),
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

StreamPartitionCommand::StreamPartitionCommand(const CommonOptions& common_opts,
                                               const std::string& topic_name,
                                               uint32_t partition_num,
                                               uint64_t start_id)
    : Command(common_opts),
      topic_name_(topic_name),
      partition_num_(partition_num),
      start_id_(start_id) { }

CommandType StreamPartitionCommand::type() const {
    return CommandType::STREAM_PARTITION;
}

const std::string StreamPartitionCommand::name() const {
    return "stream_partition";
}

const std::string& StreamPartitionCommand::topic_name() const {
    return topic_name_;
}

uint32_t StreamPartitionCommand::partition_num() const {
    return partition_num_;
}

uint64_t StreamPartitionCommand::start_id() const {
    return start_id_;
}

StreamCommand::StreamCommand(const CommonOptions& common_opts,
                                 const std::string& topic_name)
    : Command(common_opts),
      topic_name_(topic_name) { }

CommandType StreamCommand::type() const {
    return CommandType::STREAM;
}

const std::string& StreamCommand::topic_name() const {
    return topic_name_;
}

const std::string StreamCommand::name() const {
    return "stream";
}

}
