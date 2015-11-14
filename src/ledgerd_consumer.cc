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

ConsumerGroup::ConsumerGroup(unsigned int nconsumers,
                             ledger_consume_function consume_function,
                             ledger_consumer_options *options,
                             void *data) {
    ledger_status rc;

    rc = ledger_consumer_group_init(&group_,
                                    nconsumers,
                                    consume_function,
                                    options,
                                    data);
    if(rc != LEDGER_OK) {
        throw std::invalid_argument("Failed to initialize consumer group");
    }
}

ConsumerGroup::~ConsumerGroup() {
    ledger_consumer_group_close(&group_);
}

ledger_status ConsumerGroup::Attach(ledger_ctx *ctx, const std::string& topic_name,
                                    std::vector<unsigned int> partition_ids) {
    return ledger_consumer_group_attach(&group_, ctx, topic_name.c_str(), partition_ids.data());
}

ledger_status ConsumerGroup::Start() {
    return ledger_consumer_group_start(&group_);
}

ledger_status ConsumerGroup::Stop() {
    return ledger_consumer_group_stop(&group_);
}

void ConsumerGroup::Wait() {
    return ledger_consumer_group_wait(&group_);
}
}
