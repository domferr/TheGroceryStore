/**
 * Implementazione di una unbounded queue le cui operazioni sono thread-safe.
 */

#include "queue.h"
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

/**
 * Attraversa la coda in maniera ricorsiva partendo dal nodo passato per argomento e continuando per i suoi successivi.
 * Per ogni nodo, chiama la funzione passandogli il nodo stesso.
 *
 * @param node il nodo da cui partire
 * @param jobFun la funzione da chiamare per ogni nodo
 */
static void recursiveToNext(node_t *node, void (*jobFun)(void*));

/**
 * Attraversa la coda in maniera ricorsiva partendo dal nodo passato per argomento e continuando per i suoi precedenti.
 * Per ogni nodo, chiama la funzione passandogli il nodo stesso.
 *
 * @param node il nodo da cui partire
 * @param jobFun la funzione da chiamare per ogni nodo
 */
static void recursiveToPrev(node_t *node, void (*jobFun)(void*));

queue_t *queue_create(void) {
    int err;
    queue_t *queue = (queue_t*) malloc(sizeof(queue_t));
    EQNULL(queue, return NULL)
    queue->size = 0;
    queue->head = NULL;
    queue->tail = NULL;
    PTH(err, pthread_mutex_init(&(queue->mtx), NULL), free(queue); return NULL)
    PTH(err, pthread_cond_init(&(queue->empty), NULL), pthread_mutex_destroy(&(queue->mtx)); free(queue); return NULL;)

    return queue;
}

int queue_destroy(queue_t *queue) {
    int err;
    clear(queue);
    PTH(err, pthread_mutex_destroy(&(queue->mtx)), return -1)
    PTH(err, pthread_cond_destroy(&(queue->empty)), return -1)
    free(queue);
    return 0;
}

int addAtStart(queue_t *queue, void *elem) {
    int err;
    node_t *node = (node_t*) malloc(sizeof(node_t));
    EQNULL(node, return -1)

    node->elem = elem;
    node->prev = NULL;
    PTH(err, pthread_mutex_lock(&(queue->mtx)), return -1)
    node->next = queue->head;
    if (queue->head == NULL) {
        queue->tail = node;
    } else {
        (queue->head)->prev = node;
    }

    queue->head = node;
    if (queue->size == 0)
        PTH(err, pthread_cond_signal(&(queue->empty)), return -1)
    (queue->size)++;
    PTH(err, pthread_mutex_unlock(&(queue->mtx)), return -1)
    return 0;
}

int addAtEnd(queue_t *queue, void *elem) {
    int err;
    node_t *node = (node_t*) malloc(sizeof(node_t));
    EQNULL(node, return -1)

    node->elem = elem;
    node->next = NULL;

    PTH(err, pthread_mutex_lock(&(queue->mtx)), return -1)
    node->prev = queue->tail;
    if (queue->tail == NULL) {
        queue->head = node;
    } else {
        (queue->tail)->next = node;
    }

    queue->tail = node;
    if (queue->size == 0)
        PTH(err, pthread_cond_signal(&(queue->empty)), return -1)
    (queue->size)++;
    PTH(err, pthread_mutex_unlock(&(queue->mtx)), return -1)
    return 0;
}

void *removeFromStart(queue_t *queue) {
    int err;
    void *headElem;
    node_t *headNode;
    node_t *headNextNode;

    PTH(err, pthread_mutex_lock(&(queue->mtx)), return NULL)
    while (queue->size == 0) {
        PTH(err, pthread_cond_wait(&(queue->empty), &(queue->mtx)), return NULL)
    }

    headNode = (queue->head);
    headNextNode = (queue->head)->next;
    headElem = (queue->head)->elem;
    queue->head = headNextNode;
    if (queue->size == 1) {
        queue->tail = NULL;
    } else {
        headNextNode->prev = NULL;
    }
    (queue->size)--;
    free(headNode);

    PTH(err, pthread_mutex_unlock(&(queue->mtx)), return NULL)
    return headElem;
}

void removeNode(node_t *node) {

}

void *removeFromEnd(queue_t *queue) {
    int err;
    void *tailElem;
    node_t *tailNode;
    node_t *tailPrevNode;

    PTH(err, pthread_mutex_lock(&(queue->mtx)), return NULL)

    while(queue->size == 0) {
        PTH(err, pthread_cond_wait(&(queue->empty), &(queue->mtx)), return NULL)
    }


    tailNode = (queue->tail);
    tailPrevNode = (queue->tail)->prev;
    tailElem = (queue->tail)->elem;
    queue->tail = tailPrevNode;
    if (queue->size == 1) {
        queue->head = NULL;
    } else {
        tailPrevNode->next = NULL;
    }
    (queue->size)--;
    free(tailNode);

    PTH(err, pthread_mutex_unlock(&(queue->mtx)), return NULL)

    return tailElem;
}

int clear(queue_t *queue) {
    int err;
    node_t *curr;
    node_t *next;
    PTH(err, pthread_mutex_lock(&(queue->mtx)), return -1)
    curr = queue->head;
    while(curr != NULL) {
        next = curr->next;
        free(curr);
        curr = next;
    }
    queue->size = 0;
    PTH(err, pthread_mutex_unlock(&(queue->mtx)), return -1)
    return 0;
}

int applyFromFirst(queue_t *queue, void (*jobFun)(void*)) {
    int err;
    PTH(err, pthread_mutex_lock(&(queue->mtx)), return -1)
    recursiveToNext(queue->head, jobFun);
    PTH(err, pthread_mutex_unlock(&(queue->mtx)), return -1)
    return 0;
}

int applyFromLast(queue_t *queue, void (*jobFun)(void*)) {
    int err;
    PTH(err, pthread_mutex_lock(&(queue->mtx)), return -1)
    recursiveToPrev(queue->tail, jobFun);
    PTH(err, pthread_mutex_unlock(&(queue->mtx)), return -1)
    return 0;
}

static void recursiveToNext(node_t *node, void (*jobFun)(void*)) {
    if (node != NULL) {
        jobFun(node->elem);
        recursiveToNext(node->next, jobFun);
    }
}

static void recursiveToPrev(node_t *node, void (*jobFun)(void*)) {
    if (node != NULL) {
        jobFun(node->elem);
        recursiveToPrev(node->prev, jobFun);
    }
}