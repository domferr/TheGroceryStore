/**
 * Questo è lo starting point del programma
 */

#include "config.h"
#include "threadpool.h"
#include "client.h"
#include "cashier.h"
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#define DEFAULT_CONFIG_FILE "./config.txt"
#define ARG_CONFIG_FILE "-c"
#define CONFIG_FILE_NOT_VALID_MESSAGE "Il file di configurazione non è valido"

char *parseArgs(int argc, char **args);

//grocerystore -c pathtoconfigfile
int main(int argc, char** args) {
    char* configFilePath = parseArgs(argc, args);
    Config *config = (Config*) malloc(sizeof(Config));
    if (config == NULL) return -1;
    if (loadConfig(configFilePath, config) == -1) {
        free(config);
        return -1;
    }

    if (isValidConfig(config))
        printConfig(config);
    else
        printf(CONFIG_FILE_NOT_VALID_MESSAGE"\n");

    thread_pool_t *clients = thread_pool_create(config->c);
    if (clients == NULL)
        perror("Spawn clients");

    thread_pool_t *cashiers = thread_pool_create(config->k);
    if (cashiers == NULL)
        perror("Spawn cashiers");

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