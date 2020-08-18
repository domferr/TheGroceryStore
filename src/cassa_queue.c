#include "../include/utils.h"
#include "../include/cassa_queue.h"
#include <stdlib.h>

#define SERVICE_COST(ca, clq) ((ca)->fixed_service_time + ((ca)->product_service_time * (clq)->products))

//static int enqueue(cassiere_t *cassiere, client_in_queue_t *clq);
//static void dequeue(cassiere_t *cassiere, client_in_queue_t *clq);

cassa_queue_t *cassa_queue_create(void) {
    int err;
    cassa_queue_t *q = (cassa_queue_t*) malloc(sizeof(cassa_queue_t));
    EQNULL(q, return NULL)
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    q->cost = 0;
    PTH(err, pthread_cond_init(&(q->noclients), NULL), return NULL)
    return q;
}

int cassa_queue_destroy(cassa_queue_t *q) {
    int err;
    PTH(err, pthread_cond_destroy(&(q->noclients)), return -1)
    free(q);
    return 0;
}

int get_queue_cost(cassiere_t *cassiere, client_in_queue_t *clq) {
    int err, cost = 0;
    client_in_queue_t *curr;
    PTH(err, pthread_mutex_lock(&(cassiere->mutex)), return -1)
    curr = cassiere->queue->head;
    while (curr != NULL && curr != clq) {
        cost += SERVICE_COST(cassiere, curr);
        curr = curr->next;
    }
    PTH(err, pthread_mutex_unlock(&(cassiere->mutex)), return -1)
    return cost;
}

cassiere_t *get_best_queue(cassiere_t **cassieri, int k, cassiere_t *from, int from_cost) {
    cassiere_t *best = from;
    int err, i, best_cost = from_cost;
    //Paragono con il costo di tutte le casse (esclusa quella in cui si trova il cliente) e scelgo la migliore
    for (i=0; i<k; i++) {
        PTH(err, pthread_mutex_lock(&(cassieri[i]->mutex)), return NULL)
        if (cassieri[i]->isopen && cassieri[i] != from) {
            if (cassieri[i]->queue->cost < best_cost || best_cost == -1) {
                best_cost = cassieri[i]->queue->cost;
                best = cassieri[i];
            }
        }
        PTH(err, pthread_mutex_unlock(&(cassieri[i]->mutex)), return NULL)
    }
    return best;
}

int enqueue(cassiere_t *cassiere, client_in_queue_t *clq) {
    cassa_queue_t *queue = cassiere->queue;

    clq->prev = NULL;
    clq->next = queue->head;
    if (queue->head == NULL) {
        queue->tail = clq;
    } else {
        (queue->head)->prev = clq;
    }

    queue->head = clq;
    queue->size++;
    queue->cost += SERVICE_COST(cassiere, clq);
    clq->is_enqueued = 1;
    return 0;
}

void dequeue(cassiere_t *cassiere, client_in_queue_t *clq) {
    cassa_queue_t *queue = cassiere->queue;
    if (clq != NULL) {
        if (queue->head == clq)
            queue->head = clq->next;
        if (queue->tail == clq)
            queue->tail = clq->prev;
        if (clq->prev != NULL)
            clq->prev->next = clq->next;
        if (clq->next != NULL)
            clq->next->prev = clq->prev;
        queue->size--;
        queue->cost -= SERVICE_COST(cassiere, clq);
        clq->is_enqueued = 0;
        clq->prev = NULL;
        clq->next = NULL;
    }
}

int join_queue(cassiere_t *cassiere, client_in_queue_t *clq, struct timespec *queue_entrance) {
    int err, ret;
    cassa_queue_t *queue = cassiere->queue;
    PTH(err, pthread_mutex_lock(&(cassiere->mutex)), return -1)
    if (cassiere->isopen) {
        MINUS1(enqueue(cassiere, clq), return -1)
        if (queue->size == 1)
            PTH(err, pthread_cond_signal(&(cassiere->noclients)), return -1)
        clq->served = 0;
        clq->processing = 0;
        MINUS1(clock_gettime(CLOCK_MONOTONIC, queue_entrance), return -1)
        ret = 1;
    } else {
        errno = EAGAIN;
        ret = 0;
    }
    PTH(err, pthread_mutex_unlock(&(cassiere->mutex)), return -1)
    return ret;
}

cassiere_t *join_best_queue(cassiere_t *from, int from_cost, cassiere_t *to, client_in_queue_t *clq) {
    int err;
    cassiere_t *best = to;
    if (from != NULL) { //Esco dalla coda in cui mi trovo già
        PTH(err, pthread_mutex_lock(&(from->mutex)), return NULL)
        dequeue(from, clq);
        PTH(err, pthread_mutex_unlock(&(from->mutex)), return NULL)
    }
    do {
        errno = 0;
    } while(errno == EAGAIN);
    return best;
}

client_in_queue_t *get_next_client(cassiere_t *cassiere, int processing) {
    client_in_queue_t *ret;
    cassa_queue_t *queue = cassiere->queue;
    ret = queue->head;
    dequeue(cassiere, queue->head);
    ret->processing = processing;
    return ret;
}