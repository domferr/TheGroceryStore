/**
 *
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <sys/types.h>
#define MIN_T 10    //Minimo valore per il parametro T

/**
 * Struttura dati che contiene tutti i parametri di configurazione
 */
struct config {
    int k;  //numero di thread attivi come cassieri
    int kt; //tempo di gestione di un singolo prodotto da parte di un cassiere. Espresso in millisecondi
    int ka; //numero di casse aperte quando il supermercato viene inizialmente aperto
    int c;  //numero di thread attivi come clienti all'interno del supermercato
    int e;  //quanto è grande il gruppo di clienti che entra nel supermercato.
    int t;  //massimo tempo per gli acquisti per un cliente. Deve essere > 10
    int p;  //massimo numero di prodotti che acquisterà un cliente. Deve essere > 0
    int s;  //ogni quanti millisecondi il cliente decide se spostarsi o meno
    int s1; //valore soglia per chiusura di una cassa
    int s2; //valore soglia per apertura di una cassa
    int d;  //ogni quanto tempo una cassa comunica con il direttore
    char *logfilename;  //nome del file di log generato dal programma prima della sua chiusura
};

typedef struct config Config;

/**
 * Legge dati di configurazione dal path passato per argomento e ritorna la struttura di configurazione equivalente.
 *
 * @param filepath file dal quale bisogna leggere la configurazione
 * @return struttura con la configurazione contenuta nel file
 */
Config *loadConfig(char *filepath);

/**
 * Ritorna 1 se la struttura di configurazione è valida, 0 altrimenti. Una struttura di configurazione è
 * definita valida se e soltanto se tutti i parametri non sono negativi o pari a zero e vale che t > MIN_T
 * ma anche la disuguaglianza 0 < E < C.
 *
 * @param config
 * @return
 */
int isValidConfig(Config *config);

/**
 * Stampa sulla console la struttura di configurazione.
 *
 * @param config la struttura di configurazione da stampare
 */
void printConfig(Config *config);

//TODO questa documentazione
void free_config(Config *config);

#endif //CONFIG_H
