#include <iostream>
#include <memory>

#include "command_parser.h"
#include "grpc_command_executor.h"

using namespace ledgerd;

int main(int argc, char **argv) {
    CommandParser parser;
    GrpcCommandExecutor executor;

    std::unique_ptr<Command> command = parser.MakeCommand(argv, argc);
    std::unique_ptr<CommandExecutorStatus> status = executor.Execute(std::move(command));
    for(auto& line : status->lines) {
        std::cout << line << std::endl;
    }

    return static_cast<int>(status->code);
}
