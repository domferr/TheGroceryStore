#ifndef MANAGER_H
#define MANAGER_H

#include "grocerystore.h"
#include "queue.h"
#include "types.h"
#include <pthread.h>

/** Speciale struttura dati utilizzata dal manager
 * per eseguire il suo algoritmo di apertura e chiusura casse
 * */
typedef struct {
    size_t size;
    size_t active_cashiers;
    cashier_sync **ca_sync;
    int *counters;
} manager_arr_t;

/** Argomenti passati al thread manager */
typedef struct {
    grocerystore_t *gs;
    manager_queue *queue; //Su questa coda avviene la comunicazione con i notificatori
    manager_arr_t *marr;
    int s1;
    int s2;
    int ka; //numero di casse aperte all'apertura del supermercato
} manager_args;

/**
 * Funzione eseguita dal thread manager
 *
 * @param args argomenti del manager
 */
void *manager_fun(void *args);

manager_arr_t *create_manager_arr(cashier_t **cashiers, size_t size, size_t active_cashiers);

/** Distrugge gli argomenti passati al manager liberandone la memoria */
int destroy_manager_arr(manager_arr_t *arr);

/** Alloca in memoria gli argomenti da passare al manager */
manager_args *alloc_manager(grocerystore_t *gs, int s1, int s2, int ka, manager_queue *mq, cashier_t **cashiers, size_t ncash);

/** Ritorna la coda concorrente da utilizzare per le comunicazioni verso il manager.
 *  Ritorna NULL in caso di erroe ed imposta errno
 *  */
manager_queue *alloc_manager_queue(void);

/** Distrugge la coda concorrente del direttore */
int destroy_manager_queue(manager_queue *mqueue);

/**
 * Sveglia il manager dal suo stato di attesa sulla coda
 *
 * @param mqueue coda sulla quale il manager sta attendendo
 * @return 0 in caso di successo, -1 altrimenti e imposta errno
 */
int wakeup_manager(manager_queue *mqueue);

/**
 * Gestisce la notifica ricevuta e aggiorna l'array
 *
 * @param mqueue coda dalla quale prendere la notifica appena ricevuta.
 * @param marr array da aggiornare
 * @return 0 in caso di successo, -1 altrimenti e imposta errno
 */
int handle_notification(manager_queue *mqueue, manager_arr_t *marr);

#endif //MANAGER_H
