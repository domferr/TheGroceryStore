#include <stdlib.h>
#include "../include/client_in_queue.h"
#include "../include/utils.h"

client_in_queue *alloc_client_in_queue(size_t id, pthread_mutex_t *mutex) {
    int err;
    client_in_queue *cl_in_q = (client_in_queue*) malloc(sizeof(client_in_queue));
    EQNULL(cl_in_q, return NULL)

    cl_in_q->id = id;
    cl_in_q->mutex = mutex;
    cl_in_q->served = 0;
    PTH(err, pthread_cond_init(&(cl_in_q->waiting), NULL), return NULL)

    return cl_in_q;
}

int destroy_client_in_queue(client_in_queue *cl_in_q) {
    int err;
    PTH(err, pthread_cond_destroy(&(cl_in_q->waiting)), return -1)
    free(cl_in_q);
    return 0;
}

int wakeup_client(client_in_queue *clq, int served) {
    int err;
    PTH(err, pthread_mutex_lock(clq->mutex), return -1)
    clq->served = served;
    PTH(err, pthread_cond_signal(&(clq->waiting)), return -1)
    PTH(err, pthread_mutex_unlock(clq->mutex), return -1)
    return 0;
}