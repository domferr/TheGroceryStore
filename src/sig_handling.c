#include "../include/sig_handling.h"
#include "../include/utils.h"
#include "../include/scfiles.h"
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

int handle_signals(pthread_t *handler, void* (*thread_fun)(void*), void* args) {
    int err;
    struct sigaction sa;
    sigset_t set;
    //Ignoro SIGPIPE
    sa.sa_handler = SIG_IGN;
    MINUS1(sigaction(SIGPIPE, &sa, NULL), return -1)
    MINUS1(sigfillset(&set), return -1)
    MINUS1(sigdelset(&set, SIGPIPE), return -1)
    PTH(err, pthread_sigmask(SIG_SETMASK, &set, NULL), return -1)
    //Creo il thread handler. Gestirà determinati segnali chiamando la sigwait
    PTH(err, pthread_create(handler, NULL, thread_fun, args), return -1)
    return 0;
}

void *thread_sig_handler_fun(void *args) {
    int sig, err, *h_pipe = (int*) args;
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGQUIT);
    sigaddset(&set, SIGHUP);
    //Aspetta sul set che arrivi uno dei segnali di cui sopra
    PTH(err, sigwait(&set, &sig), perror("sigwait"); return NULL)
    //Invio il segnale sulla pipe e termino
    MINUS1(writen(h_pipe[1], &sig, sizeof(int)), perror("writen"); return NULL)
    return 0;
}