#ifndef MANAGER_H
#define MANAGER_H

#include "grocerystore.h"
#include "queue.h"
#include "types.h"
#include <pthread.h>

typedef struct {
    size_t size;
    cashier_sync **ca_sync;
    int **counters;
} manager_arr_t;

/** Argomenti passati al thread manager */
typedef struct {
    grocerystore_t *gs;
    manager_queue *queue; //Su questa coda avviene la comunicazione con i notificatori
    manager_arr_t *marr;
    int s1;
    int s2;
} manager_args;

/**
 * Funzione eseguita dal thread manager
 *
 * @param args argomenti del manager
 */
void *manager_fun(void *args);
manager_arr_t *create_manager_arr(cashier_t **cashiers, size_t size);
int destroy_manager_arr(manager_arr_t *arr);
manager_args *alloc_manager(grocerystore_t *gs, int s1, int s2, manager_queue *mq, cashier_t **cashiers, size_t ncash);
manager_queue *alloc_manager_queue(void);
int destroy_manager_queue(manager_queue *mqueue);
int destroy_manager(manager_args *ma);

/**
 * Sveglia il manager dal suo stato di attesa sulla coda
 *
 * @param mqueue coda sulla quale il manager sta attendendo
 * @return 0 in caso di successo, -1 altrimenti e imposta errno
 */
int wakeup_manager(manager_queue *mqueue);

int handle_notification(manager_queue *mqueue);

#endif //MANAGER_H
