#include "ledgerd_consumer.h"

namespace ledgerd {
Consumer::Consumer(ledger_consume_function consume_function,
                   ledger_consumer_options* options,
                   void *data) {
    ledger_status rc;

    rc = ledger_consumer_init(&consumer_, consume_function, options, data);
    if(rc != LEDGER_OK) {
        throw std::invalid_argument("Failed to initialize consumer");
    }
}

Consumer::~Consumer() {
    ledger_consumer_close(&consumer_);
}

ledger_status Consumer::Attach(ledger_ctx* ctx,
                               const std::string& topic_name,
                               uint32_t partition_id) {
    return ledger_consumer_attach(&consumer_, ctx, topic_name.c_str(), partition_id);
}

ledger_status Consumer::Start(uint64_t start_id) {
    return ledger_consumer_start(&consumer_, start_id);
}

ledger_status Consumer::Stop() {
    return ledger_consumer_stop(&consumer_);
}

void Consumer::Wait() {
    ledger_consumer_wait(&consumer_);
}

uint64_t Consumer::get_position() const {
    ledger_consumer_position* pos = const_cast<ledger_consumer_position*>(&consumer_.position);
    return ledger_consumer_position_get(pos);
}
}
