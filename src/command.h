#ifndef LEDGERD_COMMAND_H
#define LEDGERD_COMMAND_H

#include <string>

namespace ledgerd {

enum CommandType {
    PING,
    OPEN_TOPIC
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
}

#endif
