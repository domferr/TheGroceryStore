#include <stdlib.h>
#include "../include/client_in_queue.h"
#include "../include/utils.h"

client_in_queue_t *alloc_client_in_queue(pthread_mutex_t *mutex) {
    int err;
    client_in_queue_t *clq = (client_in_queue_t*) malloc(sizeof(client_in_queue_t));
    EQNULL(clq, return NULL)
    clq->mutex = mutex;
    clq->served = 0;
    clq->processing = 0;
    clq->products = 0;
    PTH(err, pthread_cond_init(&(clq->waiting), NULL), return NULL)

    return clq;
}

int destroy_client_in_queue(client_in_queue_t *clq) {
    int err;
    PTH(err, pthread_cond_destroy(&(clq->waiting)), return -1)
    free(clq);
    return 0;
}

int wakeup_client(client_in_queue_t *clq, int served) {
    int err;
    clq->served = served;
    clq->processing = 1;
    PTH(err, pthread_cond_signal(&(clq->waiting)), return -1)
    return 0;
}