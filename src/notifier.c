#include "utils.h"
#include "notifier.h"
#include <stdlib.h>

static int get_clients_in_queue(cashier_t *ca);

void *notifier_fun(void *args) {
    int run = 1, err, clients_in_queue;
    notifier_t *notifier = (notifier_t*) args;
    cashier_t *ca = notifier->ca;

    while(run) {
        MINUS1(msleep(notifier->interval), perror("notifier msleep"); return NULL)
        PTH(err, pthread_mutex_lock(&(notifier->mutex)), perror("notifier mutex lock"); return NULL)
        while (notifier->state == turned_off) {
            PTH(err, pthread_cond_wait(&(notifier->paused), &(notifier->mutex)), perror("notifier cond wait"); return NULL)
        }
        run = notifier->state != stopped;
        PTH(err, pthread_mutex_unlock(&(notifier->mutex)), perror("notifier mutex unlock"); return NULL)
        if (run) {
            //Prendi il numero di clienti in coda
            clients_in_queue = get_clients_in_queue(ca);
            MINUS1(clients_in_queue, perror("notifier get clients in queue"); return NULL)
            //Notifica il direttore
            MINUS1(notify(notifier->mq, ca->id, clients_in_queue), perror("notify"); return NULL)
        }
    }

    destroy_notifier(notifier);
    return 0;
}

notifier_t *alloc_notifier(cashier_t *ca, manager_queue *mq, int interval) {
    notifier_t *notifier = (notifier_t*) malloc(sizeof(notifier_t));
    EQNULL(notifier, return NULL);
    notifier->ca = ca;
    notifier->mq = mq;
    notifier->interval = interval;
    notifier->state = turned_on;
    return notifier;
}

int destroy_notifier(notifier_t *no) {
    free(no);
    return 0;
}

int notify(manager_queue *mq, size_t cashier_id, int clients_in_queue) {
    int err;
    queue_t *queue = mq->queue;

    notifier_data *data = (notifier_data*) malloc(sizeof(notifier_data));
    EQNULL(data, return -1);
    data->id = cashier_id;
    data->clients_in_queue = clients_in_queue;

    PTH(err, pthread_mutex_lock(&(mq->mutex)), return -1)
    MINUS1(push(queue, data), return -1)
    PTH(err, pthread_cond_signal(&(mq->waiting)), return -1)
    PTH(err, pthread_mutex_unlock(&(mq->mutex)), return -1)

    return 0;
}

int notifier_create(pthread_t *thread) {

}

static int get_clients_in_queue(cashier_t *ca) {
    int err, retval;
    cashier_sync *ca_sync = ca->ca_sync;
    PTH(err, pthread_mutex_lock(&(ca_sync->mutex)), return -1)
    retval = (ca->queue)->size;
    PTH(err, pthread_mutex_unlock(&(ca_sync->mutex)), return -1)
    return retval;
}