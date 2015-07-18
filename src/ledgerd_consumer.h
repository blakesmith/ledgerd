#ifndef LEDGERD_CONSUMER_H_
#define LEDGERD_CONSUMER_H_

#include <functional>

#include "consumer.h"

namespace ledgerd {
class Consumer final {
    ledger_consumer consumer_;
public:
    Consumer(ledger_consume_function consume_function,
             ledger_consumer_options* options,
             void *data);
    ~Consumer();

    ledger_status Attach(ledger_ctx *ctx, const std::string& topic_name, uint32_t partition_id);
    ledger_status Start(uint64_t start_id);
    ledger_status Stop();
    void Wait();

    uint64_t get_position() const;
};
}

#endif
