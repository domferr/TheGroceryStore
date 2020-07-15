#include "utils.h"
#include "notifier.h"
#include <stdlib.h>

void *notifier_fun(void *args) {
    notifier_t *notifier = (notifier_t*) args;
    grocerystore_t *gs = notifier->gs;
    gs_state store_state;

    NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
    while(ISOPEN(store_state)) {
        MINUS1(msleep(notifier->interval), perror("notifier msleep"); return NULL)
        notify(notifier);
        NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
    }
    free(notifier);
    return 0;
}

notifier_t *alloc_notifier(grocerystore_t *gs, cashier_t *ca, manager_queue *mq, int interval) {
    notifier_t *notifier = (notifier_t*) malloc(sizeof(notifier_t));
    EQNULL(notifier, return NULL);
    notifier->ca = ca;
    notifier->gs = gs;
    notifier->mq = mq;
    notifier->interval = interval;
    return notifier;
}

int destroy_notifier(notifier_t *no) {
    free(no);
    return 0;
}

int notify(notifier_t *no) {
    int err;
    manager_queue *mq = no->mq;
    queue_t *queue = mq->queue;
    notifier_data *data = (notifier_data*) malloc(sizeof(notifier_data));
    EQNULL(data, return -1);
    PTH(err, pthread_mutex_lock(&(mq->mutex)), return -1)
    MINUS1(push(queue, data), return -1)
    PTH(err, pthread_cond_signal(&(mq->waiting)), return -1)
    PTH(err, pthread_mutex_unlock(&(mq->mutex)), return -1)
    return 0;
}