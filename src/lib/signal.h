#ifndef LIB_LEDGER_SIGNAL_H
#define LIB_LEDGER_SIGNAL_H

#include <pthread.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
    pthread_cond_t cond;
    pthread_mutex_t lock;
} ledger_signal;

void ledger_signal_init(ledger_signal *sig);
void ledger_signal_broadcast(ledger_signal *sig);
void ledger_signal_wait(ledger_signal *sig);
void ledger_signal_wait_with_timeout(ledger_signal *sig, long int ms_timeout);

#if defined(__cplusplus)
}
#endif
#endif
