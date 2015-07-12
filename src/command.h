#ifndef LEDGERD_COMMAND_H
#define LEDGERD_COMMAND_H

#include <string>

namespace ledgerd {

enum CommandType {
    PING,
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
}

#endif
