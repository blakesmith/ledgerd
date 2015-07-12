#ifndef LEDGERD_COMMAND_PARSER_H
#define LEDGERD_COMMAND_PARSER_H

#include <memory>

#include "command.h"

namespace ledgerd {
class CommandParser final {
public:
    std::unique_ptr<Command> MakeCommand(char **argv, int argc);
};
}

#endif
