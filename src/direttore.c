#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "utils.h"

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

int main(int argc, char **args) {
    //Eseguo il parsing del nome del file di configurazione
    char *configFilePath = parse_args(argc, args);
    //Leggo il file di configurazione
    config_t *config = load_config(configFilePath);
    EQNULL(config, perror(ERROR_READ_CONFIG_FILE); exit(EXIT_FAILURE))
    if (validate(config)) {
        printf(MESSAGE_VALID_CONFIG_FILE);
        print_config(config);
    }

    free_config(config);
    msleep(10000);

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