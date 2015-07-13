#include <getopt.h>
#include <stdexcept>
#include <cstring>

#include "command_parser.h"

namespace ledgerd {

struct Options {
    std::string command_name;
    std::string topic;
    uint32_t partition_count;
};

CommandParser::CommandParser() {
    optind = 1;
}

std::unique_ptr<Command> CommandParser::MakeCommand(char **argv, int argc) {
    static struct option longopts[] = {
        { "command", required_argument, 0, 'c' },
        { "topic", required_argument, 0, 't' },
        { "partition_count", required_argument, 0, 'P' },
        { 0, 0, 0, 0 }
    };
    int ch;
    Options full_opts;

    while((ch = getopt_long(argc, argv, "c:t:P:", longopts, NULL)) != -1) {
        switch(ch) {
            case 'c':
                full_opts.command_name = std::string(optarg, strlen(optarg));
                break;
            case 't':
                full_opts.topic = std::string(optarg, strlen(optarg));
                break;
            case 'P':
                full_opts.partition_count = atoi(optarg);
                break;
            default:
                break;
        }
    }


    if(full_opts.command_name == "ping") {
        return std::unique_ptr<Command>(new PingCommand());
    } else if(full_opts.command_name == "open_topic") {
        return std::unique_ptr<Command>(new OpenTopicCommand(
                                            full_opts.topic,
                                            full_opts.partition_count));
    }

    throw std::invalid_argument("Invalid command name");
};
}
