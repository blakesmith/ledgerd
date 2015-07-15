#ifndef LEDGERD_COMMAND_EXECUTOR_H
#define LEDGERD_COMMAND_EXECUTOR_H

#include <memory>
#include <vector>

#include "command.h"

namespace ledgerd {

enum struct CommandExecutorCode {
    OK,
    ERROR
};

struct CommandExecutorStatus {
    CommandExecutorCode code;
    std::vector<std::string> lines;
};

class CommandExecutor {
public:
    virtual std::unique_ptr<CommandExecutorStatus> Execute(const Command& cmd) = 0;
};
}

#endif
