#include "signal.h"

void ledger_signal_init(ledger_signal *sig) {
    pthread_cond_init(&sig->cond, NULL);
    pthread_mutex_init(&sig->lock, NULL);
}

void ledger_signal_broadcast(ledger_signal *sig) {
    pthread_mutex_lock(&sig->lock);
    pthread_cond_broadcast(&sig->cond);
    pthread_mutex_unlock(&sig->lock);
}

void ledger_signal_wait(ledger_signal *sig) {
    pthread_mutex_lock(&sig->lock);
    pthread_cond_wait(&sig->cond, &sig->lock);
    pthread_mutex_unlock(&sig->lock);
}

void ledger_signal_wait_with_timeout(ledger_signal *sig, long int ms_timeout) {
    const struct timespec ts = {
        0,
        ms_timeout * 1000L * 1000L,
    };
    pthread_mutex_lock(&sig->lock);
    pthread_cond_timedwait(&sig->cond, &sig->lock, &ts);
    pthread_mutex_unlock(&sig->lock);
}
