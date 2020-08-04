#ifndef CASSIERE_H
#define CASSIERE_H

#include "storetypes.h"

/**
 * Funzione svolta dal thread cassiere. Ogni volta che la cassa viene aperta o chiusa il thread non termina, bensì rimane
 * in attesa che la cassa venga riaperta o che il supermercato chiuda. Si occupa della gestione dei clienti in coda in
 * fase di chiusura supermercato e di chiusura cassa.
 *
 * @param args argomenti passati alla funzione
 * @return statistiche raccolte dal thread cassiere oppure NULL in caso di errore
 */
void *cassiere_thread_fun(void *args);

/**
 * Alloca la struttura dati di un cassiere.
 *
 * @param id identificatore univoco di un cassiere. Utilizzato anche come seed per la generazione di numeri random
 * @param store puntatore alla struttura dati del supermercato
 * @param isopen flag che vale 0 se la cassa è inizialmente chiusa, 1 se è inizialmente aperta
 * @param fd descrittore del file utilizzato per notificare il direttore
 * @param fd_mtx mutex usata per notificare concorrentemente con il direttore
 * @param product_service_time quanto tempo impiega il cassiere a gestire un singolo prodotto
 * @param interval ogni quanto tempo, espresso in millisecondi, il thread notificatore del cassiere si occupa di notificare il direttore
 * @return struttura dati di un cassiere oppure NULL in caso di errore e imposta errno
 */
cassiere_t *alloc_cassiere(size_t id, store_t *store, int isopen, int fd, pthread_mutex_t *fd_mtx, int product_service_time, int interval);

/**
 * Distrugge la struttura dati di un cassiere, liberandone la memoria. Dopo la chiamata di questa funzione, il puntatore
 * alla struttura dati perde di significato e non deve essere più utilizzato.
 *
 * @param cassiere struttura dati di un cassiere
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno
 */
int cassiere_destroy(cassiere_t *cassiere);

/**
 * Imposta lo stato di una cassa in mutua esclusione
 *
 * @param cassiere struttura dati di un cassiere
 * @param open vale 1 se si vuole aprire la cassa, 0 se la si vuole chiudere
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno
 */
int set_cassa_state(cassiere_t *cassiere, int open);

/**
 * Si occupa di svegliare il cassiere passato per argomento. Il cassiere, una volta sveglio, si occuperà di verificare
 * se lo stato del supermercato e della cassa è stato cambiato.
 *
 * @param cassiere puntatore alla struttura dati di un cassiere
 * @return 0 in caso di successo, -1 altrimenti ed impsota errno
 */
int cassiere_wake_up(cassiere_t *cassiere);

#endif //CASSIERE_H
