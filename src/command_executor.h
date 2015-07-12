#ifndef LEDGERD_COMMAND_EXECUTOR_H
#define LEDGERD_COMMAND_EXECUTOR_H

#include "command.h"

namespace ledgerd {
class CommandExecutor {
public:
    virtual void Execute(const Command& cmd) = 0;
};
}

#endif
