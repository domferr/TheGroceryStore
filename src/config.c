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
#define MIN_T 10
#define IS_NULL(what, message, then)    \
        if (what == NULL) {             \
            perror(message);            \
            then;                       \
        }
#define IS_MINUS_1(what, message, then) \
        if (what == -1) {               \
            perror(message);            \
            then;                       \
        }
#define CHECK_LESS_OR_EQUAL(value, upper_limit)     \
    if (value <= upper_limit)                       \
        return 0;

int parseRow(char *row, size_t row_length, off_t *idOffset);

/**
 * Legge dati di configurazione dal path passato per argomento.
 * @param filepath da quale file bisogna leggere la configurazione
 * @return struttura con la configurazione contenuta nel file
 */
Config *load(char *filepath)
{
    char* rowBuf;
    Config *config;
    int configFD, bytesRead, valueRead;
    off_t offset = 0, idOffset = 0;

    IS_NULL((config = (Config*) malloc(sizeof(Config))), UNABLE_TO_ALLOCATE_MESSAGE, return NULL);
    IS_MINUS_1((configFD = open(filepath, O_RDONLY)), UNABLE_TO_OPEN_FILE_MESSAGE, return NULL);
    IS_NULL((rowBuf = (char*) malloc(sizeof(char)*MAX_ROW_LENGTH)), UNABLE_TO_ALLOCATE_MESSAGE, free(config); return NULL);

    //da cambiare con readn al posto di read
    while ((bytesRead = readline(configFD, rowBuf, MAX_ROW_LENGTH, &offset)) > 0) {
        if ((valueRead = parseRow(rowBuf, bytesRead, &idOffset)) == -1)
            continue;

        switch(rowBuf[idOffset])
        {
            case 'K':
                config->k = valueRead;
                break;
            case 'C':
                config->c = valueRead;
                break;
            case 'E':
                config->e = valueRead;
                break;
            case 'T':
                config->t = valueRead;
                break;
            case 'P':
                config->p = valueRead;
                break;
            case 'S':
                if (idOffset+1 < bytesRead && rowBuf[idOffset+1] == '1')
                    config->s1 = valueRead;
                else if (idOffset+1 < bytesRead && rowBuf[idOffset+1] == '2')
                    config->s2 = valueRead;
                else
                    config->s = valueRead;
                break;

        }
        printf("Config: %c -> %d\n", rowBuf[idOffset], valueRead);
    }
    IS_MINUS_1(bytesRead, UNABLE_TO_READ_FILE_MESSAGE, return NULL);

    return config;
}

/**
 * Esegue il parsing di una riga del file di configurazione. Una riga segue il formato id = valore e può contenere
 * commenti. Un commento deve essere preceduto dal carattere '#'. Il parsing ignora i commenti e setta il puntatore
 * idPtr con l'id letto e ritorna il valore ad esso corrispondente. Se la riga non è valida ritorna -1 ed il puntatore
 * idPtr non viene settato. Una riga è ritenuta non valida se non segue il formato specificato oppure se contiene solo
 * un commento.
 * @param row riga letta dal file di configurazione
 * @param row_length lunghezza della riga letta
 * @param idPtr puntatore sul quale scrivere l'id letto dal file di configurazione
 * @return il valore associato all'id letto dal file di configurazione. Ritorna -1 se la riga del file non segue il
 * corretto formato o se contiene soltanto un commento.
 */
int parseRow(char *row, size_t row_length, off_t *idOffset)
{
    char *ptr = row, *endPtr;
    size_t index = 0;
    int value;

    //Ignoro tutti gli spazi iniziali
    while (index < row_length && *ptr == ' ') { ptr++; index++; }
    //Se ho finito la riga oppure trovo un commento non vado avanti
    if (index == row_length || *ptr == '#') return -1;  //La riga contiene solo un commento oppure è vuota
    *idOffset = index;    //Questa posizione è quella dell'id

    index++;
    ptr++;
    //Vado avanti per cercare il carattere '='
    while (index < row_length && *ptr != '=' && *ptr != '#') { ptr++; index++; }
    //Se ho finito la riga oppure se trovo un commento, allora non vado avanti
    if (index == row_length || *ptr == '#') return -1;
    index++;
    ptr++;  //Da questa posizione inizia il numero

    value = strtol(ptr, &endPtr, 10);
    if (value == 0 && endPtr == ptr) return -1;    //Il numero è invalido

    return value;
}

int validate(Config *config)
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