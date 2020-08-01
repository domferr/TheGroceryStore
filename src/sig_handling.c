#include "../include/sig_handling.h"
#include "../include/utils.h"
#include <signal.h>
#include <pthread.h>
#include <unistd.h>

int handle_signals(pthread_t *handler, void* (*thread_fun)(void*), void* args) {
    int err;
    sigset_t set;
    sigemptyset(&set);
    //Segnali che non voglio ricevere
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGQUIT);
    sigaddset(&set, SIGHUP);
    //Blocco i segnali per questo thread. Da questo momento in poi, anche i thread che verranno creati non riceveranno
    //questi segnali.
    PTH(err, pthread_sigmask(SIG_BLOCK, &set, NULL), return -1)
    //Creo il thread handler. Riceverà questi segnali chiamando la sigwait
    PTH(err, pthread_create(handler, NULL, thread_fun, args), return -1)
    return 0;
}