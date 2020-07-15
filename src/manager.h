#ifndef MANAGER_H
#define MANAGER_H

#include "grocerystore.h"
#include "queue.h"
#include "cashier.h"
#include <pthread.h>

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t waiting;
    queue_t *queue;
} manager_queue;

typedef struct {
    size_t size;
    int **counters;
    int **open;
} manager_arr_t;

typedef struct {
    grocerystore_t *gs;
    manager_queue *queue; //Su questa coda avviene la comunicazione con i notificatori
    manager_arr_t *marr;
    int s1;
    int s2;
    cashier_t **cashiers;
} manager_args;

/**
 * Funzione eseguita dal thread manager
 *
 * @param args argomenti del manager
 */
void *manager_fun(void *args);

manager_args *alloc_manager(grocerystore_t *gs, int s1, int s2, cashier_t **cashiers, size_t no_of_cashiers);

/**
 * Libera l'area di memoria occupata dagli argomenti passati al manager
 *
 * @param ma argomenti passati al manager
 * @return 0 in caso di successo, -1 altrimenti e imposta errno
 */
int destroy_manager(manager_args *ma);

/**
 * Sveglia il manager dal suo stato di attesa sulla coda
 *
 * @param mqueue coda sulla quale il manager sta attendendo
 * @return 0 in caso di successo, -1 altrimenti e imposta errno
 */
int wakeup_manager(manager_queue *mqueue);

#endif //MANAGER_H
