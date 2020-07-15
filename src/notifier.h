
#ifndef NOTIFIER_H
#define NOTIFIER_H

#include "manager.h"

typedef struct {
    cashier_t *ca;  //identificatore univoco della cassa di cui notificherà i numero di clienti
    int interval;   //intervallo tra una notifica ed un'altra. espresso in millisecondi
    grocerystore_t *gs;
    manager_queue *mq;
} notifier_t;

typedef struct {
    size_t id;    //identificatore univoco della cassa
    int clients_in_queue;   //numero di clienti in coda in cassa
} notifier_data;

void *notifier_fun(void *args);
notifier_t *alloc_notifier(grocerystore_t *gs, cashier_t *ca, manager_queue *mq, int interval);
int destroy_notifier(notifier_t *no);
int notify(notifier_t *no);

#endif //NOTIFIER_H
