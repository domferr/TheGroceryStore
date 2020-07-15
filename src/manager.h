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

void *manager_fun(void *args);
manager_args *alloc_manager(grocerystore_t *gs, int s1, int s2, cashier_t **cashiers, size_t no_of_cashiers);
int destroy_manager(manager_args *ma);
int wakeup_manager(manager_queue *ma);

#endif //MANAGER_H
