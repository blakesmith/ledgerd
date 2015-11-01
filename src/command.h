#ifndef LEDGERD_COMMAND_H
#define LEDGERD_COMMAND_H

#include <string>

namespace ledgerd {

enum CommandType {
    UNKNOWN,
    PING,
    OPEN_TOPIC,
    WRITE_PARTITION,
    READ_PARTITION,
    STREAM_PARTITION
};

struct CommonOptions {
    std::string host;
    int port;
};

class Command {
    CommonOptions common_opts_;
public:
    Command(const CommonOptions& common_opts);
    const CommonOptions& common_opts() const;
    virtual CommandType type() const = 0;
    virtual const std::string name() const = 0;
};

class UnknownCommand : public Command {
    std::string command_name_;
public:
    UnknownCommand(const CommonOptions& common_opts,
                   const std::string& command_name);
    CommandType type() const;
    const std::string name() const;
    const std::string& command_name() const;
};

class PingCommand : public Command {
public:
    PingCommand(const CommonOptions& common_opts);
    CommandType type() const;
    const std::string name() const;
};

class OpenTopicCommand : public Command {
    std::string topic_name_;
    uint32_t partition_count_;
public:
    OpenTopicCommand(const CommonOptions& common_opts,
                     const std::string& topic_name,
                     uint32_t partition_count);

    CommandType type() const;
    const std::string name() const;

    const std::string& topic_name() const;
    uint32_t partition_count() const;
};

class WritePartitionCommand : public Command {
    std::string topic_name_;
    uint32_t partition_num_;
    std::string data_;
public:
    WritePartitionCommand(const CommonOptions& common_opts,
                          const std::string& topic_name,
                          uint32_t partition_num,
                          const std::string& data);
    
    CommandType type() const;
    const std::string name() const;

    const std::string& topic_name() const;
    uint32_t partition_num() const;
    const std::string& data() const;
};

class ReadPartitionCommand : public Command {
    std::string topic_name_;
    uint32_t partition_num_;
    uint64_t start_id_;
    uint32_t nmessages_;
public:
    ReadPartitionCommand(const CommonOptions& common_opts,
                         const std::string& topic_name,
                         uint32_t partition_num,
                         uint64_t start_id,
                         uint32_t nmessages);
    
    CommandType type() const;
    const std::string name() const;

    const std::string& topic_name() const;
    uint32_t partition_num() const;
    uint32_t nmessages() const;
    uint64_t start_id() const;
};

class StreamPartitionCommand : public Command {
    std::string topic_name_;
    uint32_t partition_num_;
    uint64_t start_id_;
public:
    StreamPartitionCommand(const CommonOptions& common_opts,
                           const std::string& topic_name,
                           uint32_t partition_num,
                           uint64_t start_id);

    CommandType type() const;
    const std::string name() const;

    const std::string& topic_name() const;
    uint32_t partition_num() const;
    uint64_t start_id() const;
};
}

#endif
