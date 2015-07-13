#ifndef LEDGERD_COMMAND_H
#define LEDGERD_COMMAND_H

#include <string>

namespace ledgerd {

enum CommandType {
    PING,
    OPEN_TOPIC,
    WRITE_PARTITION,
    READ_PARTITION
};

class Command {
public:
    virtual CommandType type() const = 0;
    virtual const std::string name() const = 0;
};

class PingCommand : public Command {
public:
    CommandType type() const;
    const std::string name() const;
};

class OpenTopicCommand : public Command {
    std::string topic_name_;
    uint32_t partition_count_;
public:
    OpenTopicCommand(const std::string& topic_name,
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
    WritePartitionCommand(const std::string& topic_name,
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
    ReadPartitionCommand(const std::string& topic_name,
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
}

#endif
