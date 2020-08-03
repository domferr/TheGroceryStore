#ifndef STORE_H
#define STORE_H

#include "storetypes.h"
#define ISOPEN(st) (st) == open_state

/**
 * Crea la struttura dati dello store. Richiede il numero massimo di clienti che possono esserci contemporaneamente
 * all'interno del supermercato.
 *
 * @param c numero massimo di clienti che possono esserci contemporaneamente all'interno del supermercato
 * @param e quanto è grande il gruppo di clienti che può entrare nel supermercato
 * @return puntatore alla struttura dati allocata, NULL in caso di errore e setta errno
 */
store_t *store_create(size_t c, size_t e);

/**
 * Distrugge la struttura del supermercato passata per argomento. Il puntatore passato, dopo l'esecuzione di questa
 * funzione non ha più significato. La memoria viene totalmente liberata.
 *
 * @param store struttura dati del supermercato di cui liberare la memoria
 * @return 0 se la distruzione è avvenuta con successo, -1 altrimenti e setta errno
 */
int store_destroy(store_t *store);

/**
 * Aggiorna il puntatore passato per argomento con lo stato del supermercato. Ritorna 0 in caso di successo, -1 altrimenti
 * @param store struttura dati del supermercato
 * @param state puntatore alla variabile da aggiornare con lo stato del supermercato
 * @return 0 in caso di successo, -1 altrimenti
 */
int get_store_state(store_t *store, store_state *state);

int close_store(store_t *store, store_state closing_state);

int enter_store(store_t *store);

int exit_store(store_t *store);

#endif //STORE_H
