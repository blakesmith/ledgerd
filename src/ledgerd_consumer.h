#ifndef LEDGERD_CONSUMER_H_
#define LEDGERD_CONSUMER_H_

#include <functional>
#include <vector>

#include "consumer.h"

namespace ledgerd {
class Consumer final {
    ledger_consumer consumer_;
public:
    Consumer(ledger_consume_function consume_function,
             ledger_consumer_options* options,
             void *data);
    ~Consumer();

    ledger_status Attach(ledger_ctx *ctx,
                         const std::string& topic_name,
                         uint32_t partition_id);
    ledger_status Start(uint64_t start_id);
    ledger_status Stop();
    void Wait();

    uint64_t get_position() const;
};

class ConsumerGroup final {
    ledger_consumer_group group_;
public:
    ConsumerGroup(unsigned int nconsumers,
                  ledger_consume_function consume_function,
                  ledger_consumer_options *options,
                  void *data);
    ~ConsumerGroup();

    ledger_status Attach(ledger_ctx *ctx,
                         const std::string& topic_name,
                         std::vector<unsigned int> partition_ids);
    ledger_status Start();
    ledger_status Stop();
    void Wait();
};
}

#endif
