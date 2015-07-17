#ifndef LEDGERD_CONSUMER_H_
#define LEDREGD_CONSUMER_H_

#include <functional>

#include "consumer.h"

namespace ledgerd {
using ConsumeFunction = std::function<ledger_consume_function(ledger_consumer_ctx *,
                                                              ledger_message_set *,
                                                              void *)>;
class Consumer final {
    ledger_consumer consumer_;
public:
    Consumer(ConsumeFunction consume_function,
             ledger_consumer_options* options,
             void *data);
    ~Consumer();

    ledger_status Attach(ledger_ctx *ctx, const std::string& topic_name, uint32_t partition_id);
    ledger_status Start(uint64_t start_id);
    ledger_status Stop();
};
}

#endif
