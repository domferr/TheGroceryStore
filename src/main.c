/**
 * Questo è lo starting point del programma.
 */

#include "utils.h"
#include "grocerystore.h"
#include "config.h"
#include "threadpool.h"
#include "signal_handler.h"
#include "client.h"
#include "cashier.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>

#define DEFAULT_CONFIG_FILE "./config.txt"
#define ARG_CONFIG_FILE "-c"
#define ERROR_READ_CONFIG_FILE "Unable to read config file"
#define MESSAGE_CONFIG_FILE_NOT_VALID "Il file di configurazione presenta valori non corretti o mancanti\n"
#define MESSAGE_STORE_IS_OPEN "Supermercato aperto!\n"
#define MESSAGE_STORE_IS_CLOSING "\rChiusura del supermercato...\n"

char *parseArgs(int argc, char **args);
int setup_signal_handling(pthread_t *handler, grocerystore_t *gs);
thread_pool_t *cashiers_create(grocerystore_t *gs, int size);
thread_pool_t *clients_create(grocerystore_t *gs, int size, int t, int p);

//grocerystore -c pathtoconfigfile
int main(int argc, char** args) {
    int err;
    grocerystore_t *gs;
    pthread_t handler_thread;            //Thread che gestisce i segnali
    thread_pool_t *clients;
    thread_pool_t *cashiers;
    void **clients_logs;
    void **cashiers_logs;
    char *configFilePath = parseArgs(argc, args);
    Config *config = loadConfig(configFilePath);
    EQNULL(config, perror(ERROR_READ_CONFIG_FILE); exit(EXIT_FAILURE))

    if (isValidConfig(config)) {
        printConfig(config);
    } else {
        printf(MESSAGE_CONFIG_FILE_NOT_VALID);
        printConfig(config);
        free(config);
        exit(EXIT_FAILURE);
    }

    gs = grocerystore_create(config->c);
    EQNULL(gs, perror("grocerystore_create"); exit(EXIT_FAILURE))

    NOTZERO(setup_signal_handling(&handler_thread, gs), perror("setup_signal_handling"); exit(EXIT_FAILURE))

    cashiers = cashiers_create(gs, config->k);
    EQNULL(cashiers, perror("cashiers_create"); exit(EXIT_FAILURE))

    clients = clients_create(gs, config->c, config->t, config->p);
    EQNULL(clients, perror("clients_create"); exit(EXIT_FAILURE))

    printf(MESSAGE_STORE_IS_OPEN);
    gs_state ending_state = doBusiness(gs, config->c, config->e);
    switch(ending_state) {
        case open:  //stato non possibile
        case gserror:
            perror("Main");
            free(gs);
            free(config);
            exit(EXIT_FAILURE);
            break;
        case closed:
            printf(MESSAGE_STORE_IS_CLOSING);
            printf("Tutti i clienti in coda verranno serviti prima di chiudere definitivamente.\n");
            break;
        case closed_fast:
            printf(MESSAGE_STORE_IS_CLOSING);
            printf("Tutti i clienti in coda verranno fatti uscire immediatamente.\n");
            break;
    }
    //join!
    PTH(err, pthread_join(handler_thread, NULL), exit(EXIT_FAILURE))
    clients_logs = thread_pool_join(clients);
    EQNULL(clients_logs, perror("join clients"); exit(EXIT_FAILURE))
    cashiers_logs = thread_pool_join(cashiers);
    EQNULL(cashiers_logs, perror("join cashiers"); exit(EXIT_FAILURE))
    //cleanup!
    NOTZERO(thread_pool_free(clients), perror("join clients"); exit(EXIT_FAILURE))
    NOTZERO(thread_pool_free(cashiers), perror("join cashiers"); exit(EXIT_FAILURE))
    free(clients_logs);
    free(cashiers_logs);
    free(gs);
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

int setup_signal_handling(pthread_t *handler, grocerystore_t *gs) {
    int error;
    signal_handler_t *handler_args = malloc(sizeof(signal_handler_t));
    if (handler_args == NULL) return -1;

    sigemptyset(&(handler_args->set));
    //Segnali che non voglio ricevere
    sigaddset(&(handler_args->set), SIGINT);
    sigaddset(&(handler_args->set), SIGQUIT);
    sigaddset(&(handler_args->set), SIGHUP);

    //Blocco i segnali per questo thread. Da questo momento in poi, anche i thread che verranno creati non riceveranno
    //questi segnali.
    error = pthread_sigmask(SIG_BLOCK, &(handler_args->set), NULL);
    if (error != 0) {
        errno = error;
        free(handler_args);
        return -1;
    }

    handler_args->gs = gs;
    //Creo il thread handler. Riceverà questi segnali chiamando la sigwait
    if (pthread_create(handler, NULL, &thread_handler_fun, handler_args) != 0) {
        free(handler_args);
        return -1;
    }
    return 0;
}


thread_pool_t *cashiers_create(grocerystore_t *gs, int size) {
    int i;
    thread_pool_t *cashiers = thread_pool_create(size);
    EQNULL(cashiers, return NULL)

    for (i = 0; i < size; ++i) {
        cashier_t *cashier = alloc_cashier(i, gs, pause, 20);
        EQNULL(cashier, return NULL)

        NOTZERO(thread_create(cashiers, &cashier_fun, cashier), return NULL)
    }
    return cashiers;
}

thread_pool_t *clients_create(grocerystore_t *gs, int size, int t, int p) {
    int i;
    thread_pool_t *clients = thread_pool_create(size);
    EQNULL(clients, return NULL)

    for (i = 0; i < size; ++i) {
        client_t *client = alloc_client(i, gs, t, p);
        EQNULL(client, return NULL)

        NOTZERO(thread_create(clients, &client_fun, client), return NULL)
    }
    return clients;
}
