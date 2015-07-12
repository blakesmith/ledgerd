#include "command_parser.h"

namespace ledgerd {
std::unique_ptr<Command> CommandParser::MakeCommand(char **argv, int argc) {
    return std::unique_ptr<Command>(new PingCommand());
}
}
