//
// Created by Domenico on 03/07/2020.
//

#include "config.h"
#include "scfiles.h"
#include "stdio.h"
#include "stdlib.h"
#include <fcntl.h>
#include <unistd.h>

#define UNABLE_TO_ALLOCATE_MESSAGE "Impossibile allocare memoria"
#define UNABLE_TO_OPEN_FILE_MESSAGE "Impossibile aprire il file di configurazione"
#define UNABLE_TO_READ_FILE_MESSAGE "Impossibile leggere il file di configurazione"
#define MAX_ROW_LENGTH 128

#define IS_NULL(what, message, then)    \
        if (what == NULL) {             \
            perror(message);            \
            then;                       \
        }                               \

#define IS_MINUS_1(what, message, then) \
        if (what == -1) {               \
            perror(message);            \
            then;                       \
        }                               \

#define CHECK_LESS_OR_EQUAL(value, upper_limit)     \
    if (value <= upper_limit)                       \
        return 0;                                   \

struct config* load(char* filename)
{
    char* rowBuf;
    struct config* config;
    int configFD, bytesRead;
    off_t offset = 0;

    IS_NULL((config = (struct config*) malloc(sizeof(struct config))), UNABLE_TO_ALLOCATE_MESSAGE, return NULL);
    IS_MINUS_1((configFD = open(filename, O_RDONLY)), UNABLE_TO_OPEN_FILE_MESSAGE, return NULL);
    IS_NULL((rowBuf = (char*) malloc(sizeof(char)*MAX_ROW_LENGTH)), UNABLE_TO_ALLOCATE_MESSAGE, return NULL);

    //da cambiare con readn al posto di read
    while ((bytesRead = readline(configFD, rowBuf, MAX_ROW_LENGTH, &offset)) > 0) {
        printf("Riga letta: %s\n", rowBuf);
        parseRow(rowBuf, bytesRead, config);
        printf("\n");
    }
    IS_MINUS_1(bytesRead, UNABLE_TO_READ_FILE_MESSAGE, return NULL);

    return config;
}

void parseRow(char *row, int row_length, struct config *config)
{
    char *idPtr = NULL, *ptr = row, *endPtr;
    size_t index = 0;
    int value;

    //Ignoro tutti gli spazi iniziali
    while (index < row_length && *ptr == ' ') { ptr++; index++; }
    //Se ho finito la riga oppure trovo un commento non vado avanti
    if (index == row_length || *ptr == '#') return;
    idPtr = ptr;    //Questa posizione è quella dell'id

    //Vado avanti per cercare il carattere '='
    index++;
    ptr++;
    //Ignoro tutti gli spazi
    while (index < row_length && *ptr == ' ') { ptr++; index++; }
    //Se ho finito la riga oppure se non ho trovato il carattere '=' oppure se trovo un commento, allora non vado avanti
    if (index == row_length || *ptr != '=' || *ptr == '#') return;
    index++;
    ptr++;  //Da questa posizione inizia il numero

    char id = idPtr[0];
    value = strtol(ptr, &endPtr, 10);
    if (value == 0 && endPtr == ptr) return;    //Il numero è invalido

    printf("Config: %c -> %d\n", id, value);
}

int validate(struct config* config)
{
    //Deve valere: k > 0
    CHECK_LESS_OR_EQUAL(config->k, 0);
    //Deve valere: p > 0
    CHECK_LESS_OR_EQUAL(config->p, 0);
    //Deve valere che 0 < E < C
    if (config->c <= 0 || config->e <= 0 || config->e >= config->c)
        return 0;
    //Deve valere: t > 10
    CHECK_LESS_OR_EQUAL(config->t, MIN_T);

    return 1;
}