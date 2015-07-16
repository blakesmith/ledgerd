#include <getopt.h>
#include <stdexcept>
#include <cstring>

#include "command_parser.h"

namespace ledgerd {

struct Options {
    CommonOptions common_opts;
    std::string command_name;
    std::string topic;
    std::string data;
    uint32_t partition_count;
    uint32_t partition_num;
    uint32_t nmessages;
    uint64_t start_id;
};

CommandParser::CommandParser() {
    optind = 1;
}

std::unique_ptr<Command> CommandParser::MakeCommand(char **argv, int argc) {
    static struct option longopts[] = {
        { "help", no_argument, 0, 'H' },
        { "host", required_argument, 0, 'h' },
        { "port", required_argument, 0, 'p' },
        { "command", required_argument, 0, 'c' },
        { "topic", required_argument, 0, 't' },
        { "partition_count", required_argument, 0, 'C' },
        { "partition", required_argument, 0, 'P' },
        { "data", required_argument, 0, 'd' },
        { "start", required_argument, 0, 's' },
        { "nmessages", required_argument, 0, 'n' },
        { 0, 0, 0, 0 }
    };
    int ch;
    Options full_opts;

    full_opts.common_opts.host = "localhost";
    full_opts.common_opts.port = 64399;
    full_opts.nmessages = 1;
    full_opts.start_id = 0;

    while((ch = getopt_long(argc, argv, "h:p:c:t:P:C:d:s:n:", longopts, NULL)) != -1) {
        switch(ch) {
            case 'H':
                return std::unique_ptr<Command>(new UnknownCommand(
                                                    full_opts.common_opts,
                                                    ""));
            case 'h':
                full_opts.common_opts.host = std::string(optarg, strlen(optarg));
                break;
            case 'p':
                full_opts.common_opts.port = atoi(optarg);
                break;
            case 'c':
                full_opts.command_name = std::string(optarg, strlen(optarg));
                break;
            case 't':
                full_opts.topic = std::string(optarg, strlen(optarg));
                break;
            case 'C':
                full_opts.partition_count = atoi(optarg);
                break;
            case 'P':
                full_opts.partition_num = atoi(optarg);
                break;
            case 'd':
                full_opts.data = std::string(optarg, strlen(optarg));
                break;
            case 's':
                full_opts.start_id = atoi(optarg);
                break;
            case 'n':
                full_opts.nmessages = atoi(optarg);
                break;
            default:
                break;
        }
    }


    if(full_opts.command_name == "ping") {
        return std::unique_ptr<Command>(new PingCommand(
                                            full_opts.common_opts));
    } else if(full_opts.command_name == "open_topic") {
        return std::unique_ptr<Command>(new OpenTopicCommand(
                                            full_opts.common_opts,
                                            full_opts.topic,
                                            full_opts.partition_count));
    } else if(full_opts.command_name == "write_partition") {
        return std::unique_ptr<Command>(new WritePartitionCommand(
                                            full_opts.common_opts,
                                            full_opts.topic,
                                            full_opts.partition_num,
                                            full_opts.data));
    } else if(full_opts.command_name == "read_partition") {
        return std::unique_ptr<Command>(new ReadPartitionCommand(
                                            full_opts.common_opts,
                                            full_opts.topic,
                                            full_opts.partition_num,
                                            full_opts.start_id,
                                            full_opts.nmessages));
    }

    return std::unique_ptr<Command>(new UnknownCommand(
                                        full_opts.common_opts,
                                        full_opts.command_name));
};
}
