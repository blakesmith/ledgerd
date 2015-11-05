#include "command_executor.h"

namespace ledgerd {
CommandExecutorStatus::CommandExecutorStatus()
    : _stream_open(true),
      _has_next(false) { }

CommandExecutorStatus::CommandExecutorStatus(const CommandExecutorCode code)
    : _code(code),
      _stream_open(true),
      _has_next(false) { }

bool CommandExecutorStatus::HasNext() {
    std::lock_guard<std::mutex> lock(_lock);
    return _has_next;
}

std::vector<std::string> CommandExecutorStatus::Next() {
    std::lock_guard<std::mutex> lock(_lock);
    std::vector<std::string> lines = _lines;
    _lines.clear();
    _has_next = false;
    return lines;
}

bool CommandExecutorStatus::StreamIsOpen() {
    std::lock_guard<std::mutex> lock(_lock);
    return _stream_open;
}

void CommandExecutorStatus::Close() {
    std::lock_guard<std::mutex> lock(_lock);
    flush();
    _stream_open = false;
}

void CommandExecutorStatus::AddLine(const std::string& line) {
    std::lock_guard<std::mutex> lock(_lock);
    _lines.push_back(line);
}

void CommandExecutorStatus::Flush() {
    std::lock_guard<std::mutex> lock(_lock);
    flush();
}

void CommandExecutorStatus::flush() {
    _has_next = true;
}

const CommandExecutorCode CommandExecutorStatus::code() {
    std::lock_guard<std::mutex> lock(_lock);
    return _code;
}

void CommandExecutorStatus::set_code(const CommandExecutorCode code) {
    std::lock_guard<std::mutex> lock(_lock);
    _code = code;
}
}
