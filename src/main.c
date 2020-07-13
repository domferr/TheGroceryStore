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
int setup_signal_handling(pthread_t *handler, grocerystore_t *gs, cashier_t **cashiers_args, size_t no_of_cashiers);
thread_pool_t *cashiers_create(cashier_t **cashiers_args, grocerystore_t *gs, int size);
thread_pool_t *clients_create(grocerystore_t *gs, int size, int t, int p);

//grocerystore -c pathtoconfigfile
int main(int argc, char** args) {
    int err, i;
    grocerystore_t *gs;
    gs_state closing_state;         //Stato di chiusura del supermercato
    pthread_t handler_thread;       //Thread che gestisce i segnali
    thread_pool_t *clients;
    thread_pool_t *cashiers;
    cashier_t **cashiers_args;
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

    cashiers_args = (cashier_t**) malloc(sizeof(cashier_t*)*config->k);
    EQNULL(cashiers_args, perror("cashiers_args"); exit(EXIT_FAILURE))
    for (i = 0; i < config->k; i++) {
        cashier_state starting_state = sleeping;
        cashiers_args[i] = alloc_cashier(i, gs, starting_state, 20);    //TODO prendere il vero product service time al posto di 20
        EQNULL(cashiers_args[i], perror("alloc_cashier"); exit(EXIT_FAILURE))
    }
    err = setup_signal_handling(&handler_thread, gs, cashiers_args, config->k);
    NOTZERO(err, perror("setup_signal_handling"); exit(EXIT_FAILURE))

    cashiers = cashiers_create(cashiers_args, gs, config->k);
    EQNULL(cashiers, perror("cashiers_create"); exit(EXIT_FAILURE))

    clients = clients_create(gs, config->c, config->t, config->p);
    EQNULL(clients, perror("clients_create"); exit(EXIT_FAILURE))

    printf(MESSAGE_STORE_IS_OPEN);
    err = manage_entrance(gs, &closing_state, config->c, config->e);
    NOTZERO(err, perror("Main"); exit(EXIT_FAILURE))

    if (closing_state == closed) {
        printf(MESSAGE_STORE_IS_CLOSING);
        printf("Tutti i clienti in coda verranno serviti prima di chiudere definitivamente.\n");
    } else if (closing_state == closed_fast) {
        printf(MESSAGE_STORE_IS_CLOSING);
        printf("Tutti i clienti in coda verranno fatti uscire immediatamente.\n");
    }

    //join!
    PTH(err, pthread_join(handler_thread, NULL), perror("join thread handler"); exit(EXIT_FAILURE))    //join sul thread signal handler
    clients_logs = thread_pool_join(clients);   //join sui clienti
    EQNULL(clients_logs, perror("join clients"); exit(EXIT_FAILURE))
    printf("Clienti terminati\n");
    cashiers_logs = thread_pool_join(cashiers); //join sui cassieri
    EQNULL(cashiers_logs, perror("join cashiers"); exit(EXIT_FAILURE))
    printf("Cassieri terminati\n");

    //cleanup!
    NOTZERO(thread_pool_free(clients), perror("free clients"); exit(EXIT_FAILURE))  //free clienti
    NOTZERO(thread_pool_free(cashiers), perror("free cashiers"); exit(EXIT_FAILURE)) //free cassieri
    free(cashiers_args);
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

int setup_signal_handling(pthread_t *handler, grocerystore_t *gs, cashier_t **cashiers_args, size_t no_of_cashiers) {
    int err;
    signal_handler_t *handler_args = malloc(sizeof(signal_handler_t));
    EQNULL(handler_args, return -1)

    sigemptyset(&(handler_args->set));
    //Segnali che non voglio ricevere
    sigaddset(&(handler_args->set), SIGINT);
    sigaddset(&(handler_args->set), SIGQUIT);
    sigaddset(&(handler_args->set), SIGHUP);

    //Blocco i segnali per questo thread. Da questo momento in poi, anche i thread che verranno creati non riceveranno
    //questi segnali.
    PTH(err, pthread_sigmask(SIG_BLOCK, &(handler_args->set), NULL), free(handler_args); return -1)

    handler_args->gs = gs;
    handler_args->cashiers = cashiers_args;
    handler_args->no_of_cashiers = no_of_cashiers;
    //Creo il thread handler. Riceverà questi segnali chiamando la sigwait
    PTH(err, pthread_create(handler, NULL, &thread_handler_fun, handler_args), return -1)

    return 0;
}


thread_pool_t *cashiers_create(cashier_t **cashiers_args, grocerystore_t *gs, int size) {
    int i;
    thread_pool_t *cashiers = thread_pool_create(size);
    EQNULL(cashiers, return NULL)

    for (i = 0; i < size; ++i) {
        NOTZERO(thread_create(cashiers, &cashier_fun, cashiers_args[i]), return NULL)
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
