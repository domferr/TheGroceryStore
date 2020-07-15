
#ifndef NOTIFIER_H
#define NOTIFIER_H

#include "cashier.h"
#include "manager.h"

typedef enum {
    turned_on,
    turned_off,
    stopped
} notifier_state;

typedef struct {
    pthread_t thread;
    cashier_t *ca;  //identificatore univoco della cassa di cui notificherà i numero di clienti
    int interval;   //intervallo tra una notifica ed un'altra. espresso in millisecondi
    manager_queue *mq;
    pthread_cond_t paused;
    notifier_state state;
    pthread_mutex_t mutex;
} notifier_t;

typedef struct {
    size_t id;    //identificatore univoco della cassa
    int clients_in_queue;   //numero di clienti in coda in cassa
} notifier_data;

void *notifier_fun(void *args);
notifier_t *alloc_notifier(cashier_t *ca, manager_queue *mq, int interval);
int destroy_notifier(notifier_t *no);
int notify(manager_queue *mq, size_t cashier_id, int clients_in_queue);

#endif //NOTIFIER_H
