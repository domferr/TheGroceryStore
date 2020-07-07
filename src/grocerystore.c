/**
 * Questo è lo starting point del programma
 */

#include "config.h"
#include "executorspool.h"
#include "queue.h"
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#define DEFAULT_CONFIG_FILE "./config.txt"
#define ARG_CONFIG_FILE "-c"
#define CONFIG_FILE_NOT_VALID_MESSAGE "Il file di configurazione non è valido"

char *parseArgs(int argc, char **args);
void print(void *elem);

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

    executors_pool_t *clients = executors_pool_create(config->c);
    if (clients == NULL)
        perror("Spawn clients");
    queue_t *queue = queue_create();
    if (queue == NULL)
        perror("queue_create");
    int prova = 20;
    int prova2 = 30;
    int prova3 = 40;
    addAtStart(queue, (&prova));
    addAtStart(queue, (&prova2));
    addAtStart(queue, (&prova3));
    applyFromFirst(queue, &print);
    int rimosso = *((int*)((void*)removeFromStart(queue)));
    printf("%d ", rimosso);
    rimosso = *((int*)((void*)removeFromStart(queue)));
    printf("%d ", rimosso);
    rimosso = *((int*)((void*)removeFromStart(queue)));
    printf("%d ", rimosso);
    applyFromFirst(queue, &print);

    free(config);
    return 0;
}

void print(void *elem) {
    int *value = (void*) elem;
    printf("%d\n", *value);
}

/**
 * Esegue il parsing degli argomenti passati al programma via linea di comando. Ritorna il path del file
 * di configurazione indicato dall'utente oppure il path di default se l'utente non ha specificato nulla.
 * @param argc numero di argomenti
 * @param args gli argomenti passati al programma
 * @return il path del file di configurazione indicato dall'utente oppure il path di default se l'utente non lo ha
 * specificato
 */
char *parseArgs(int argc, char **args)
{
    int i = 1;
    while(i < argc && strcmp(args[i], ARG_CONFIG_FILE) != 0) {
        i++;
    }
    i++;
    return i < argc ? args[i]:(char *)DEFAULT_CONFIG_FILE;
}