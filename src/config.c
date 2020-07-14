#include "config.h"
#include "utils.h"
#include "scfiles.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

//Costanti
#define MAX_ROW_LENGTH 128  //Massima dimensione del buffer per leggere una riga del file

#define CHECK_LESS_OR_EQUAL(value, upper_limit)     \
    if (value <= upper_limit)                       \
        return 0;

/**
 * Esegue il parsing di una riga del file di configurazione. Una riga segue il formato id = valore e può contenere
 * commenti. Un commento deve essere preceduto dal carattere '#'. Il parsing ignora i commenti e setta l'offset idOffset
 * con la posizione dell'id e ritorna il valore ad esso corrispondente. Se la riga non è valida ritorna -1 ed l'offset
 * idOffset non viene settato. Una riga è ritenuta non valida se non segue il formato specificato oppure se contiene solo
 * un commento.
 *
 * @param row riga letta dal file di configurazione
 * @param row_length lunghezza della riga letta
 * @param idOffset viene settato con la posizione dell'id della riga letta
 * @return il valore associato all'id letto dal file di configurazione. Ritorna -1 se la riga del file non segue il
 * corretto formato o se contiene soltanto un commento.
 *///TODO update this doc with @param char *value_ptr
static int parseRow(char *row, size_t row_length, off_t *idOffset, char **value_ptr) {
    char *ptr = row;
    size_t index = 0;

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
    *value_ptr = ptr;
    /*value = strtol(ptr, &endPtr, 10);
    if (value == 0 && endPtr == ptr) return -1;    //Il numero è invalido*/

    return 0;
}

static int set_int_value(int *dest, char *value_ptr);

Config *loadConfig(char *filepath) {
    Config *config = (Config*) malloc(sizeof(Config));
    EQNULL(config, return NULL);
    int configFD, bytesRead;
    off_t offset = 0, idOffset = 0;
    char *rowBuf, *value_ptr;
    size_t len;

    EQNULL((rowBuf = (char*) malloc(sizeof(char) * MAX_ROW_LENGTH)), free(config); return NULL);
    MINUS1((configFD = open(filepath, O_RDONLY)), free(config); free(rowBuf); return NULL);

    while ((bytesRead = readline(configFD, rowBuf, MAX_ROW_LENGTH, &offset)) > 0) {
        MINUS1(parseRow(rowBuf, bytesRead, &idOffset, &value_ptr), continue)

        switch(rowBuf[idOffset])
        {
            case 'K':
                if (idOffset+1 < bytesRead && rowBuf[idOffset+1] == 'T')
                    set_int_value(&(config->kt), value_ptr);
                else if (idOffset+1 < bytesRead && rowBuf[idOffset+1] == 'A')
                    set_int_value(&(config->ka), value_ptr);
                else
                    set_int_value(&(config->k), value_ptr);
                break;
            case 'C':
                set_int_value(&(config->c), value_ptr);
                break;
            case 'E':
                set_int_value(&(config->e), value_ptr);
                break;
            case 'T':
                set_int_value(&(config->t), value_ptr);
                break;
            case 'P':
                set_int_value(&(config->p), value_ptr);
                break;
            case 'S':
                if (idOffset+1 < bytesRead && rowBuf[idOffset+1] == '1')
                    set_int_value(&(config->s1), value_ptr);
                else if (idOffset+1 < bytesRead && rowBuf[idOffset+1] == '2')
                    set_int_value(&(config->s2), value_ptr);
                else
                    set_int_value(&(config->s), value_ptr);
                break;
            case 'L':
                len = strcspn(value_ptr, " #");
                config->logfilename = (char *) malloc((sizeof(char*)*len)+1);
                EQNULL(config->logfilename, return NULL)
                config->logfilename = strncpy(config->logfilename, value_ptr, len);
                (config->logfilename)[len] = '\0';
                break;

        }
    }
    free(rowBuf);
    MINUS1(close(configFD), free(config); return NULL);
    MINUS1(bytesRead, free(config); return NULL);

    return config;
}

static int set_int_value(int *dest, char *value_ptr) {
    char *endPtr;
    int value = strtol(value_ptr, &endPtr, 10);
    if (value == 0 && endPtr == value_ptr) return -1;
    *dest = value;
    return 0;
}

int isValidConfig(Config *config) {
    //Deve valere: K > 0
    CHECK_LESS_OR_EQUAL(config->k, 0);
    //Deve valere: P > 0
    CHECK_LESS_OR_EQUAL(config->p, 0);
    //Deve valere: S > 0
    CHECK_LESS_OR_EQUAL(config->s, 0);
    //Deve valere: S1 > 0
    CHECK_LESS_OR_EQUAL(config->s1, 0);
    //Deve valere: S2 > 0
    CHECK_LESS_OR_EQUAL(config->s2, 0);
    //Deve valere: 0 < E < C
    if (config->c <= 0 || config->e <= 0 || config->e >= config->c)
        return 0;
    //Deve valere: 0 < KA <= K
    if (config->ka <= 0 || config->ka > config->k)
        return 0;
    //Il file di log non deve essere mancante
    if ((config->logfilename)[0] == '\0')
        return 0;
    //Deve valere: T > 10
    CHECK_LESS_OR_EQUAL(config->t, MIN_T);

    return 1;
}

void printConfig(Config *config) {
    printf("{K: %d, C: %d, E: %d, T: %d, P: %d, S: %d, S1: %d, S2: %d}\n",
            config->k, config->c, config->e, config->t, config->p, config->s, config->s1, config->s2);
}

void free_config(Config *config) {
    free(config->logfilename);
    free(config);
}