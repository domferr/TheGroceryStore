#include "utils.h"
#include "client_in_queue.h"
#include "cashier.h"
#include <stdio.h>
#include <stdlib.h>

client_in_queue *alloc_client_in_queue(size_t id) {
    int err;
    client_in_queue *cl_in_q = (client_in_queue*) malloc(sizeof(client_in_queue));
    EQNULL(cl_in_q, return NULL)

    PTH(err, pthread_mutex_init(&(cl_in_q->mutex), NULL), return NULL)
    PTH(err, pthread_cond_init(&(cl_in_q->waiting), NULL), return NULL)

    cl_in_q->id = id;

    return cl_in_q;
}

int destroy_client_in_queue(client_in_queue *cl_in_q) {
    int err;
    PTH(err, pthread_mutex_destroy(&(cl_in_q->mutex)), return -1)
    PTH(err, pthread_cond_destroy(&(cl_in_q->waiting)), return -1)
    free(cl_in_q);
    return 0;
}