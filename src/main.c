/**
 * Questo è lo starting point del programma.
 */
#define _POSIX_C_SOURCE 200809L
#include "grocerystore.h"
#include "config.h"
#include "threadpool.h"
#include "client.h"
#include "signal_handler.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>

#define DEFAULT_CONFIG_FILE "./config.txt"
#define ARG_CONFIG_FILE "-c"
#define CONFIG_FILE_NOT_VALID_MESSAGE "Il file di configurazione presenta valori non corretti o mancanti"

char *parseArgs(int argc, char **args);
int setup_signal_handling(pthread_t *handler, grocerystore_t *gs);
thread_pool_t *clients_create(grocerystore_t *gs, int size);

//grocerystore -c pathtoconfigfile
int main(int argc, char** args) {
    grocerystore_t *gs;
    pthread_t handler_thread;            //Thread che gestisce i segnali
    void **clients_logs;
    char* configFilePath = parseArgs(argc, args);
    Config *config = loadConfig(configFilePath);
    if (config == NULL)
        return -1;
    if (isValidConfig(config)) {
        printConfig(config);
    } else {
        printf(CONFIG_FILE_NOT_VALID_MESSAGE"\n");
        printConfig(config);
        free(config);
        return -1;
    }

    gs = grocerystore_create(config->e);
    if (gs == NULL)  {
        free(config);
        return -1;
    }
    if (setup_signal_handling(&handler_thread, gs) != 0) {
        perror("setup_signal_handling");
        free(gs);
        free(config);
        return -1;
    }

    thread_pool_t *clients = clients_create(gs, config->c);
    if (clients == NULL) {
        perror("clients_create");
        free(gs);
        free(config);
        return 0;
    }
    printf("Supermercato aperto!\n");
    gs_state ending_state = doBusiness(gs, config->c, config->e);
    printf("Chiusura del supermercato...\n");
    if (ending_state == closed_fast) {
        printf("Tutti i clienti in coda verranno fatti uscire senza essere serviti\n");
    }

    pthread_join(handler_thread, NULL);
    clients_logs = thread_pool_join(clients);
    if (clients_logs == NULL) {
        printf("E' avvenuto un problema inaspettato. Non sarà possibile avere i log dei clienti\n");
    }
    thread_pool_free(clients);
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

thread_pool_t *clients_create(grocerystore_t *gs, int size) {
    int i;
    thread_pool_t *clients = thread_pool_create(size);
    if (clients == NULL) {
        perror("thread_pool_create - clients");
        return NULL;
    }
    for (i = 0; i < size; ++i) {
        client_t *client = alloc_client(i, gs);
        if (client == NULL) {
            thread_pool_free(clients);
            return NULL;
        }
        if (thread_create(clients, &client_fun, client) != 0) {
            perror("thread_create");
            return NULL;
        }
    }
    return clients;
}
