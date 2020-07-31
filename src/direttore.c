#include "../include/sig_handling.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include "../include/config.h"
#include "../include/utils.h"

#define STORE_EXECUTABLE_PATH "./bin/supermercato"
#define STORE_EXECUTABLE_NAME "supermercato"
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
char *parse_args(int argc, char **args);
static void *thread_handler_fun(void *args);
static int fork_store(char *config_file, pid_t *pid);

int main(int argc, char **args) {
    int err;
    pid_t pid_store;
    pthread_t handler_thread;   //thread che si occupa di gestire i segnali

    //Eseguo il parsing del nome del file di configurazione
    char *config_file_path = parse_args(argc, args);
    //Lancio il processo supermercato
    MINUS1(fork_store(argc, config_file_path, &pid_store), perror("fork_store"); exit(EXIT_FAILURE))
    //Gestione dei segnali mediante thread apposito
    MINUS1(handle_signals(&handler_thread, &thread_handler_fun, (void*)&pid_store), perror("handle_signals"); exit(EXIT_FAILURE))
    //Leggo il file di configurazione
    config_t *config = load_config(config_file_path);
    EQNULL(config, perror(ERROR_READ_CONFIG_FILE); exit(EXIT_FAILURE))
    if (!validate(config))
        exit(EXIT_FAILURE);
    printf(MESSAGE_VALID_CONFIG_FILE);
    print_config(config);

    //join sul thread signal handler
    PTH(err, pthread_join(handler_thread, NULL), perror("join thread handler"); exit(EXIT_FAILURE))
    //cleanup
    free_config(config);
    printf("\rDIRETTORE: Goodbye!\n");
    return 0;
}

char *parse_args(int argc, char **args) {
    int i = 1;
    while(i < argc && strcmp(args[i], ARG_CONFIG_FILE) != 0) {
        i++;
    }
    i++;
    return i < argc ? args[i]:(char *)DEFAULT_CONFIG_FILE;
}

static int fork_store(char *config_file, pid_t *pid) {
    *pid = fork();
    switch (*pid) {
        case 0:
            //corrisponde ad esempio a ./bin/supermercato -c config.txt
            execl(STORE_EXECUTABLE_PATH, STORE_EXECUTABLE_NAME, config_file);
            //Se exec ritorna allora c'è stato sicuramente errore ed errno è stato impostato
            return -1;
        case -1:
            return -1;
        default: //il padre, quindi il direttore, prosegue
            break;
    }

    return 0;
}

static void *thread_handler_fun(void *args) {
    int sig;
    pid_t *pid_store = (pid_t*) args;
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
    MINUS1(kill(*pid_store, sig), perror("kill"); exit(EXIT_FAILURE))
    return 0;
}