/**
 * Questo è lo starting point del programma
 */
#define _POSIX_C_SOURCE 200809L
#include "config.h"
#include "threadpool.h"
#include "client.h"
#include "cashier.h"
#include "signal_handler.h"
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define DEFAULT_CONFIG_FILE "./config.txt"
#define ARG_CONFIG_FILE "-c"
#define CONFIG_FILE_NOT_VALID_MESSAGE "Il file di configurazione non è valido"

char *parseArgs(int argc, char **args);
int setup_signal_handling(pthread_t *handler);

//grocerystore -c pathtoconfigfile
int main(int argc, char** args) {
    pthread_t handler_thread;            //Thread che gestisce i segnali
    char* configFilePath = parseArgs(argc, args);
    Config *config = loadConfig(configFilePath);
    if (config == NULL) return -1;

    if (isValidConfig(config))
        printConfig(config);
    else
        printf(CONFIG_FILE_NOT_VALID_MESSAGE"\n");

    if (setup_signal_handling(&handler_thread) != 0) {
        perror("setup_signal_handling");
        return 0;
    }

    thread_pool_t *clients = thread_pool_create(config->c);
    if (clients == NULL)
        perror("thread_pool_create - clients");

    thread_pool_t *cashiers = thread_pool_create(config->k);
    if (cashiers == NULL)
        perror("thread_pool_create - cashiers");

    cashier_t *ca1 = alloc_cashier(1000);
    if (ca1 == NULL) {
        perror("alloc_cashier");
        return 0;
    }
    if (thread_create(cashiers, &cashier, ca1) != 0)
        perror("thread_create");
    /*if (thread_create(clients, &client, NULL) != 0)
        perror("thread_create");
    if (thread_create(clients, &client, NULL) != 0)
        perror("thread_create");
    if (thread_create(clients, &client, NULL) != 0)
        perror("thread_create");*/

    thread_pool_join(clients);
    thread_pool_free(clients);
    thread_pool_join(cashiers);
    thread_pool_free(cashiers);
    free_cashier(ca1);
    pthread_join(handler_thread, NULL);
    free(config);
    return 0;
}


/**
 * Esegue il parsing degli argomenti passati al programma via linea di comando. Ritorna il path del file
 * di configurazione indicato dall'utente oppure il path di default se l'utente non ha specificato nulla.
 * @param argc numero di argomenti
 * @param args gli argomenti passati al programma
 * @return il path del file di configurazione indicato dall'utente oppure il path di default se l'utente non lo ha
 * specificato
 */
char *parseArgs(int argc, char **args) {
    int i = 1;
    while(i < argc && strcmp(args[i], ARG_CONFIG_FILE) != 0) {
        i++;
    }
    i++;
    return i < argc ? args[i]:(char *)DEFAULT_CONFIG_FILE;
}

int setup_signal_handling(pthread_t *handler) {
    signal_handler_t *handler_args = malloc(sizeof(signal_handler_t));

    sigemptyset(&(handler_args->set));
    //Segnali che non voglio ricevere
    sigaddset(&(handler_args->set), SIGINT);
    sigaddset(&(handler_args->set), SIGQUIT);
    sigaddset(&(handler_args->set), SIGHUP);

    //Blocco i segnali per questo thread. Da questo momento in poi, anche i thread che verranno creati non riceveranno
    //questi segnali.
    if (pthread_sigmask(SIG_BLOCK, &(handler_args->set), NULL) != 0) {
        free(handler_args);
        return -1;
    }

    //Creo il thread handler. Riceverà questi segnali chiamando la sigwait
    if (pthread_create(handler, NULL, &signal_handler_fun, handler_args) != 0) {
        free(handler_args);
        return -1;
    }
    return 0;
}