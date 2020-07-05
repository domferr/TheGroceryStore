/**
 * Questo è lo starting point del programma
 */

#include "config.h"
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#define DEFAULT_CONFIG_FILE "./config.txt"

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
        printf("Il file di configurazione non è valido\n");

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
char *parseArgs(int argc, char **args)
{
    int i = 1;
    while(i < argc && strcmp(args[i], "-c") != 0) {
        i++;
    }
    i++;
    return i < argc ? args[i]:DEFAULT_CONFIG_FILE;
}