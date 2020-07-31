#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "../include/config.h"
#include "../include/utils.h"

#define COMMENT_CHAR '#'    //Carattere utilizzato per iniziare un commento
#define ASSIGNMENT_CHAR '='    //Carattere utilizzato per indicare il valore da assegnare ad un parametro
#define MAX_ROW_LENGTH 128  //Massima dimensione del buffer per leggere una riga del file
#define MISSING -1
#define LESS_OR_EQUAL(value, upper_limit, val_id)                               \
    if ((value) <= (upper_limit)) {                                                 \
        fprintf(stderr, "File di configurazione non valido: "val_id" mancante oppure minore o uguale a %d\n", upper_limit); \
        return 0;                                                               \
    }                                                                           \

/**
 * Ritorna una struttura di configurazione vuota con tutti i parametri mancanti. Ritorna NULL in caso di errore
 * e imposta errno.
 *
 * @return struttura di configurazione con tutti i parametri mancanti oppure NULL in caso di errore
 */
static config_t* alloc_config(void);

/**
 * Esegue il parsing di una riga del file di configurazione. Una riga segue il formato id ASSIGNMENT_CHAR valore
 * (ad esempio id = valore) e può contenere commenti. Un commento deve essere preceduto dal carattere indicato
 * da COMMENT_CHAR (ad esempio '#'). Il parsing ignora i commenti e setta l'offset id_off con la posizione dell'id
 * e setta la posizione del valore corrispondente in val_off.
 * Se la riga non è valida, la funzione ritorna 0. Altrimenti ritorna 1.
 * Una riga è ritenuta non valida se non segue il formato specificato oppure se contiene solo un commento o è vuota.
 *
 * @param row riga letta dal file di configurazione
 * @param id_off viene settato con la posizione dell'id del parametro all'interno della riga letta
 * @param val_off viene settato con la posizione del valore del parametro all'interno della riga letta
 * @return 0 se la riga letta presenta solo un commento o è vuota, 1 altrimenti e imposta id_off e val_off
 */
static int parse_row(char *row, off_t *id_off, off_t *val_off);

/**
 * Imposta dest con il valore intero equivalente alla stringa passata per argomento. Se la stringa non presenta alcun
 * valore intero, dest non viene impostato.
 *
 * @param dest puntatore ad una variabile intera
 * @param str puntatore ad una stringa
 */
static void string_to_int(int *dest, char *str);

config_t *load_config(char *path) {
    size_t len;
    char row[MAX_ROW_LENGTH], *value_ptr;
    FILE *file;
    off_t id_off, val_off;
    config_t *config = alloc_config();
    EQNULL(config, return NULL)
    EQNULL((file = fopen(path, "r")), return NULL)

    while(fgets(row, MAX_ROW_LENGTH, file) != NULL) {
        if (parse_row(row, &id_off, &val_off) == 0) continue;
        value_ptr = row + val_off;
        switch(row[id_off]) {
            case 'K':
                if (row[id_off+1] == 'T')
                    string_to_int(&(config->kt), value_ptr);
                else if (row[id_off+1] == 'A')
                    string_to_int(&(config->ka), value_ptr);
                else
                    string_to_int(&(config->k), value_ptr);
                break;
            case 'C':
                string_to_int(&(config->c), value_ptr);
                break;
            case 'E':
                string_to_int(&(config->e), value_ptr);
                break;
            case 'T':
                string_to_int(&(config->t), value_ptr);
                break;
            case 'P':
                string_to_int(&(config->p), value_ptr);
                break;
            case 'S':
                if (row[id_off+1] == '1')
                    string_to_int(&(config->s1), value_ptr);
                else if (row[id_off+1] == '2')
                    string_to_int(&(config->s2), value_ptr);
                else
                    string_to_int(&(config->s), value_ptr);
                break;
            case 'L':
                len = strcspn(value_ptr, " #\n");
                config->logfilename = (char *) malloc(sizeof(char)*(len+1));
                EQNULL(config->logfilename, return NULL)
                config->logfilename = strncpy(config->logfilename, value_ptr, len);
                (config->logfilename)[len] = '\0';
                break;
            case 'D':
                string_to_int(&(config->d), value_ptr);
                break;

        }
    }

    fclose(file);
    return config;
}

static int parse_row(char *row, off_t *id_off, off_t *val_off) {
    char *ptr = row;
    *id_off = 0;
    *val_off = 0;

    //Ignoro tutti gli eventuali spazi iniziali
    while (*ptr == ' ') { ptr++; (*id_off)++;}
    //Se esco dal while perchè ho finito la riga oppure trovo un commento, allora non vado avanti e ritorno 0
    if (*ptr == '\0' || *ptr == COMMENT_CHAR) return 0;

    //Questa posizione è quella dell'id. Adesso vado avanti cercando il valore da assegnare
    ptr++;
    *val_off = *id_off + 1;
    //Vado avanti per cercare il carattere '='
    while (*ptr != ASSIGNMENT_CHAR && *ptr != COMMENT_CHAR) { ptr++; (*val_off)++; }
    //Se esco dal while perchè ho finito la riga oppure se trovo un commento, allora non vado avanti
    if (*ptr == '\0' || *ptr == COMMENT_CHAR) return 0;

    //In questo momento sono sul carattere '=' quindi il valore inizierà subito dopo.
    (*val_off)++;
    ptr++;
    //Ignoro tutti gli eventuali spazi che sono presenti prima del valore
    while (*ptr == ' ') { ptr++; (*val_off)++;}
    //Se esco dal while perchè ho finito la riga oppure se trovo un commento, allora ritorno 0
    if (*ptr == '\0' || *ptr == COMMENT_CHAR) return 0;

    return 1;
}

static config_t* alloc_config(void) {
    config_t *config = (config_t*) malloc(sizeof(config_t));
    EQNULL(config, return NULL)

    //Imposto tutti i parametri come mancanti
    config->k = MISSING;
    config->kt = MISSING;
    config->ka = MISSING;
    config->c = MISSING;
    config->d = MISSING;
    config->e = MISSING;
    config->p = MISSING;
    config->s = MISSING;
    config->s1 = MISSING;
    config->s2 = MISSING;
    config->t = MISSING;
    config->logfilename = NULL;

    return config;
}

static void string_to_int(int *dest, char *str) {
    char *endPtr;
    int value = strtol(str, &endPtr, 10);
    if (endPtr != str)
        *dest = value;
}

int validate(config_t *config) {
    EQNULL(config, fprintf(stderr, "Struttura di configurazione nulla\n"); return 0)
    //Deve valere: K > 0
    LESS_OR_EQUAL(config->k, 0, "K")
    //Deve valere: KA > 0
    LESS_OR_EQUAL(config->ka, 0, "KA")
    //Deve valere: 0 < KA <= K
    if (config->ka > config->k) {
        fprintf(stderr, "File di configurazione non valido: Deve valere: 0 < KA <= K\n");
        return 0;
    }
    //Deve valere: C > 0
    LESS_OR_EQUAL(config->c, 0, "C")
    //Deve valere: E > 0
    LESS_OR_EQUAL(config->e, 0, "E")
    //Deve valere: 0 < E < C
    if (config->e >= config->c) {
        fprintf(stderr, "File di configurazione non valido: Deve valere: 0 < E < C\n");
        return 0;
    }
    //Deve valere: KT > 0
    LESS_OR_EQUAL(config->kt, 0, "KT")
    //Deve valere: T > 10
    LESS_OR_EQUAL(config->t, MIN_T, "T")
    //Deve valere: P > 0
    LESS_OR_EQUAL(config->p, 0, "P")
    //Deve valere: S > 0
    LESS_OR_EQUAL(config->s, 0, "S")
    //Deve valere: S1 > 0
    LESS_OR_EQUAL(config->s1, 0, "S1")
    //Deve valere: S2 > 0
    LESS_OR_EQUAL(config->s2, 0, "S2")
    //Deve valere: D > 0
    LESS_OR_EQUAL(config->d, 0, "D")
    //Il file di log non deve essere mancante e deve avere significato
    if (config->logfilename == NULL || (config->logfilename)[0] == '\0') {
        fprintf(stderr, "File di configurazione non valido: Il nome del file di log è mancante oppure è privo di significato\n");
        return 0;
    }

    return 1;
}

void free_config(config_t *config) {
    if (config != NULL) {
        if (config->logfilename != NULL)
            free(config->logfilename);
        free(config);
    }
}

void print_config(config_t *config) {
    printf("{K: %d, KT: %d, KA: %d, C: %d, E: %d, T: %d, P: %d, S: %d, S1: %d, S2: %d, D: %d, log file: %s}\n",
           config->k, config->kt, config->ka, config->c, config->e, config->t,
           config->p, config->s, config->s1, config->s2, config->d, config->logfilename);
}