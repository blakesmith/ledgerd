#include "command.h"

namespace ledgerd {
CommandType PingCommand::type() const {
    return CommandType::PING;
}

const std::string PingCommand::name() const {
    return "ping";
}
}
