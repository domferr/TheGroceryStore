#ifndef MANAGER_H
#define MANAGER_H

#include "grocerystore.h"
#include "queue.h"
#include <pthread.h>

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t waiting;
    queue_t *queue;
} manager_queue;

/** Argomenti passati al thread manager */
typedef struct {
    grocerystore_t *gs;
    manager_queue *queue; //Su questa coda avviene la comunicazione con i notificatori
    int s1;
    int s2;
} manager_args;

typedef struct {
    size_t id;    //identificatore univoco della cassa
    int clients_in_queue;   //numero di clienti in coda in cassa
} notification_data;

/**
 * Funzione eseguita dal thread manager
 *
 * @param args argomenti del manager
 */
void *manager_fun(void *args);

int destroy_manager(manager_args *ma);

/**
 * Sveglia il manager dal suo stato di attesa sulla coda
 *
 * @param mqueue coda sulla quale il manager sta attendendo
 * @return 0 in caso di successo, -1 altrimenti e imposta errno
 */
int wakeup_manager(manager_queue *mqueue);

int notify_manager(manager_queue *mqueue, size_t cashier_id, int clients_in_queue);

int handle_notification(manager_queue *mqueue);

#endif //MANAGER_H
