#include <gtest/gtest.h>

#include <atomic>

#include "signal.h"

static std::atomic<bool> signaled(false);

static void *leader_exec(void *signal_ptr) {
    ledger_signal *sig = (ledger_signal *)signal_ptr;

    sleep(1);
    signaled = true;
    ledger_signal_broadcast(sig);
}

namespace ledger_signal_test {
TEST(LedgerSignal, BasicWait) {
    ledger_signal sig;
    pthread_t leader_thread;

    ledger_signal_init(&sig);
    pthread_create(&leader_thread, NULL, leader_exec, &sig);
    ledger_signal_wait(&sig);
    ASSERT_TRUE(signaled);
}
}
