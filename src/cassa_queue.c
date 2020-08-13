#include <stdlib.h>
#include "../include/cassa_queue.h"
#include "../include/utils.h"

cassa_queue_t *cassa_queue_create(void) {
    int err;
    cassa_queue_t *q = (cassa_queue_t*) malloc(sizeof(cassa_queue_t));
    EQNULL(q, return NULL)
    q->first = NULL;
    q->last = NULL;
    q->size = 0;
    q->totprod = 0;
    PTH(err, pthread_cond_init(&(q->noclients), NULL), return NULL)
    return q;
}

int cassa_queue_destroy(cassa_queue_t *q) {
    int err;
    PTH(err, pthread_cond_destroy(&(q->noclients)), return -1)
    free(q);
    return 0;
}

int get_queue_cost(cassiere_t *ca, int enqueued, int totprod) {
    return (enqueued * ca->fixed_service_time)+(ca->product_service_time * totprod);
}

cassiere_t *get_best_queue(cassiere_t **cassieri, int k, cassiere_t *from) {
    cassiere_t *best = NULL;
    int err, i, cost, best_cost = -1;
    for (i=0; i<k; i++) {
        PTH(err, pthread_mutex_lock(&(cassieri[i]->mutex)), return NULL)
        if (cassieri[i]->isopen && cassieri[i] != from) {
            cost = get_queue_cost(cassieri[i], cassieri[i]->queue->size, 0);
            if (cost < best_cost || best_cost == -1) {
                best_cost = cost;
                best = cassieri[i];
            }
        }
        PTH(err, pthread_mutex_unlock(&(cassieri[i]->mutex)), return NULL)
    }
    return best;
}

cassiere_t *join_best_queue(cassiere_t *best, client_in_queue_t *clq, cassiere_t *from) {
    int err;
    if (from != NULL) {
        PTH(err, pthread_mutex_lock(&(from->mutex)), return NULL)
        if (from->isopen) {

        }
        PTH(err, pthread_mutex_unlock(&(from->mutex)), return NULL)
    }
    return best;
}

int join_queue(cassa_queue_t *q, client_in_queue_t *clq) {
    int err, done = 0;
    client_in_queue_t *last = q->last;
    /*
    PTH(err, pthread_mutex_lock(&(q->mtx)), return -1)
    if (q->first == NULL) {
        q->first = clq;
        q->last = clq;
        q->size++;
        done = 1;
    }
    PTH(err, pthread_mutex_unlock(&(q->mtx)), return -1)

    if (!done) {
        PTH(err, pthread_mutex_lock(last->mutex), return -1)
        q->last->next = clq;
        q->last = clq;
        PTH(err, pthread_mutex_unlock(last->mutex), return -1)
    }*/
    return 0;
}

int leave_queue(cassa_queue_t *q, client_in_queue_t *clq) {
    int err;
    if (clq == q->first) {
        PTH(err, pthread_mutex_lock(clq->mutex), return -1)
        q->first = q->first->next;
        q->size--;
        PTH(err, pthread_mutex_unlock(clq->mutex), return -1)
    }

    return 0;
}