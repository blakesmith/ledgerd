#ifndef LEDGERD_COMMAND_EXECUTOR_H
#define LEDGERD_COMMAND_EXECUTOR_H

#include <memory>
#include <mutex>
#include <queue>

#include "command.h"

namespace ledgerd {

enum struct CommandExecutorCode {
    OK = 0,
    ERROR = -1
};

class CommandExecutorStatus {
    CommandExecutorCode _code;
    std::vector<std::string> _lines;
    bool _stream_open;
    bool _has_next;
    std::mutex _lock;

    void flush();
public:
    CommandExecutorStatus();
    CommandExecutorStatus(const CommandExecutorCode code);

    bool HasNext();
    std::vector<std::string> Next();
    bool StreamIsOpen();

    void Close();
    void AddLine(const std::string& line);
    void Flush();

    const CommandExecutorCode code();
    void set_code(const CommandExecutorCode code);
};

class CommandExecutor {
public:
    virtual std::unique_ptr<CommandExecutorStatus> Execute(std::unique_ptr<Command> cmd) = 0;
};
}

#endif
