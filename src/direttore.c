#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>
#include "../include/config.h"
#include "../include/utils.h"

#define DEFAULT_CONFIG_FILE "./config.txt"
#define ARG_CONFIG_FILE "-c"
#define ERROR_READ_CONFIG_FILE "Impossibile leggere il file di configurazione"
#define MESSAGE_VALID_CONFIG_FILE "File di configurazione: "

/**
 * Esegue il parsing degli argomenti passati al programma via linea di comando. Ritorna il path del file
 * di configurazione indicato dall'utente oppure il path di default se l'utente non ha specificato nulla.
 * @param argc numero di argomenti
 * @param args gli argomenti passati al programma
 * @return il path del file di configurazione indicato dall'utente oppure il path di default se l'utente non lo ha
 * specificato
 */
static char *parse_args(int argc, char **args);
static int handle_signals(pthread_t *handler);
static void *thread_handler_fun(void *args);

int main(int argc, char **args) {
    int err;
    pthread_t handler_thread;
    MINUS1(handle_signals(&handler_thread), perror("handle_signals"); exit(EXIT_FAILURE))

    //Eseguo il parsing del nome del file di configurazione
    char *configFilePath = parse_args(argc, args);
    //Leggo il file di configurazione
    config_t *config = load_config(configFilePath);
    EQNULL(config, perror(ERROR_READ_CONFIG_FILE); exit(EXIT_FAILURE))
    if (!validate(config))
        exit(EXIT_FAILURE);

    printf(MESSAGE_VALID_CONFIG_FILE);
    print_config(config);

    msleep(2000);
    //join sul thread signal handler
    PTH(err, pthread_join(handler_thread, NULL), perror("join thread handler"); exit(EXIT_FAILURE))
    free_config(config);
    return 0;
}

static int handle_signals(pthread_t *handler) {
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
    PTH(err, pthread_create(handler, NULL, &thread_handler_fun, NULL), return -1)
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

static char *parse_args(int argc, char **args) {
    int i = 1;
    while(i < argc && strcmp(args[i], ARG_CONFIG_FILE) != 0) {
        i++;
    }
    i++;
    return i < argc ? args[i]:(char *)DEFAULT_CONFIG_FILE;
}
