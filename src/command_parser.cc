#include <getopt.h>
#include <stdexcept>
#include <cstring>

#include "command_parser.h"

namespace ledgerd {

struct Options {
    std::string command_name;
    std::string topic;
    std::string data;
    uint32_t partition_count;
    uint32_t partition_num;
};

CommandParser::CommandParser() {
    optind = 1;
}

std::unique_ptr<Command> CommandParser::MakeCommand(char **argv, int argc) {
    static struct option longopts[] = {
        { "command", required_argument, 0, 'c' },
        { "topic", required_argument, 0, 't' },
        { "partition_count", required_argument, 0, 'P' },
        { "partition", required_argument, 0, 'p' },
        { "data", required_argument, 0, 'd' },
        { 0, 0, 0, 0 }
    };
    int ch;
    Options full_opts;

    while((ch = getopt_long(argc, argv, "c:t:P:p:d:", longopts, NULL)) != -1) {
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
            case 'p':
                full_opts.partition_num = atoi(optarg);
                break;
            case 'd':
                full_opts.data = std::string(optarg, strlen(optarg));
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
    } else if(full_opts.command_name == "write_partition") {
        return std::unique_ptr<Command>(new WritePartitionCommand(
                                            full_opts.topic,
                                            full_opts.partition_num,
                                            full_opts.data));
    }

    throw std::invalid_argument("Invalid command name");
};
}
