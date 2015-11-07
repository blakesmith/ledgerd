#include <iostream>
#include <memory>

#include "command_parser.h"
#include "grpc_command_executor.h"

using namespace ledgerd;

int main(int argc, char **argv) {
    CommandParser parser;
    GrpcCommandExecutor executor;

    std::unique_ptr<Command> command = parser.MakeCommand(argc, argv);
    std::unique_ptr<CommandExecutorStatus> status = executor.Execute(std::move(command));
    do {
        while(status->HasNext()) {
            for(auto& line : status->Next()) {
                std::cout << line << std::endl;
            }
        }
    } while(status->StreamIsOpen());

    executor.Stop();
    return static_cast<int>(status->code());
}
