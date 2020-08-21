#include "../include/sig_handling.h"
#include "../include/utils.h"
#include "../include/scfiles.h"
#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

int handle_signals(void* (*thread_fun)(void*), void* args) {
    int err;
    pthread_t handler;
    sigset_t set;
    struct sigaction sa;  //Ignoro SIGPIPE
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    MINUS1(sigaction(SIGPIPE, &sa, NULL), return -1)
    MINUS1(sigfillset(&set), return -1)
    MINUS1(sigdelset(&set, SIGPIPE), return -1)
    PTH(err, pthread_sigmask(SIG_SETMASK, &set, NULL), return -1)
    //Creo il thread handler. Gestirà determinati segnali chiamando la sigwait
    PTH(err, pthread_create(&handler, NULL, thread_fun, args), return -1)
    PTH(err, pthread_detach(handler), return -1)
    return 0;
}

void *thread_sig_handler_fun(void *args) {
    int sig, err, *h_pipe = (int*) args;
    sigset_t set;
    MINUS1ERR(sigemptyset(&set), return NULL)
    MINUS1ERR(sigaddset(&set, SIGINT), return NULL)
    MINUS1ERR(sigaddset(&set, SIGQUIT), return NULL)
    MINUS1ERR(sigaddset(&set, SIGHUP), return NULL)
    //Aspetta sul set che arrivi uno dei segnali di cui sopra
    PTHERR(err, sigwait(&set, &sig), return NULL)
    //Invio il segnale sulla pipe e termino
    if(fcntl(h_pipe[1], F_GETFL) >= 0)
        MINUS1ERR(writen(h_pipe[1], &sig, sizeof(int)), return NULL)
    return 0;
}