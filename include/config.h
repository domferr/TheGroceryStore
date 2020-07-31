#ifndef CONFIG_H
#define CONFIG_H

#define MIN_T 10            //Minimo valore per il parametro T

/**
 * Struttura dati che contiene tutti i parametri di configurazione
 */
typedef struct {
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
} config_t;

/**
 * Legge dati di configurazione dal path passato per argomento e ritorna la struttura di configurazione equivalente.
 * Il valore di ritorno è NULL in caso di errore e errno viene impostato. In caso di successo, la struttura ritornata
 * potrebbe non essere valida se il file di configurazione non presenta tutti i parametri o se alcuni parametri sono
 * invalidi. Utilizzare la funzione validate(config_t *config) per conoscere la struttura è valida o meno.
 *
 * @param path Da quale path bisogna leggere la configurazione
 * @return Struttura con la configurazione contenuta nel file oppure NULL in caso di errore e imposta errno
 */
config_t *load_config(char *path);

/**
 * Ritorna 1 se la struttura di configurazione è valida, 0 altrimenti e stampa sullo stderr qual è il problema.
 * Una struttura di configurazione è definita valida se:
 * - nessun parametro è mancante
 * - tutti i parametri non sono negativi o pari a zero
 * - t > MIN_T
 * - 0 < E < C
 *
 * @param config Struttura di configurazione da validare
 * @return 1 se la struttura è valida, 0 alrimenti e stampa sullo stderr il parametro non valido.
 */
int validate(config_t *config);

/**
 * Libera la memoria occupata dalla struttura di configurazione passata per argomento. Dopo la chiamata di questa
 * funzione, il puntatore alla struttura di configurazione perde di significato e non deve essere più utilizzato.
 *
 * @param config struttura di configurazione di cui liberare la memoria
 */
void free_config(config_t *config);

/**
 * Stampa sul terminale la struttura di configurazione.
 *
 * @param config la struttura di configurazione da stampare
 */
void print_config(config_t *config);

#endif //CONFIG_H
