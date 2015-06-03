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
