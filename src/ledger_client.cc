#include <iostream>
#include <chrono>
#include <memory>
#include <thread>

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
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    } while(status->StreamIsOpen());

    executor.Stop();
    return static_cast<int>(status->code());
}
