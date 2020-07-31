#include "../include/sig_handling.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "../include/config.h"
#include "../include/utils.h"

static void *thread_handler_fun(void *args);

int main(int argc, char **args) {
    int err;
    pthread_t handler_thread;
    if (argc != 2) {
        printf("Usage: Il processo supermercato deve essere eseguito dal direttore\n");
        exit(EXIT_FAILURE);
    }
    //Gestione dei segnali mediante thread apposito
    MINUS1(handle_signals(&handler_thread, &thread_handler_fun, NULL), perror("handle_signals"); exit(EXIT_FAILURE))
    //Leggo file di configurazione
    config_t *config = load_config(args[1]);
    EQNULL(config, exit(EXIT_FAILURE))
    if (!validate(config))
        exit(EXIT_FAILURE);


    //START --- Ciclo di vita
    printf("store is running\n");
    //END   --- Ciclo di vita


    //join sul thread signal handler
    PTH(err, pthread_join(handler_thread, NULL), perror("join thread handler"); exit(EXIT_FAILURE))
    //cleanup
    free_config(config);
    printf("\rSUPERMERCATO: Goodbye!\n");
    return 0;
}

static void *thread_handler_fun(void *args) {
    int sig;
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGQUIT);
    sigaddset(&set, SIGHUP);
    //Aspetta sul set che arrivi un segnale
    NOTZERO(sigwait(&set, &sig), perror("sigwait"); return NULL)
    //Gestione del segnale arrivato
    switch (sig) {
        case SIGINT:
        case SIGQUIT:
            break;
        case SIGHUP:
            break;
        default:
            break;
    }
    return 0;
}