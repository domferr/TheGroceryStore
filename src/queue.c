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
    queue_t *queue = (queue_t*) malloc(sizeof(queue_t));
    EQNULL(queue, return NULL)
    queue->size = 0;
    queue->head = NULL;
    queue->tail = NULL;
    return queue;
}

int queue_destroy(queue_t *queue) {
    clear(queue);
    free(queue);
    return 0;
}

int push(queue_t *queue, void *elem) {
    node_t *node = (node_t*) malloc(sizeof(node_t));
    EQNULL(node, return -1)
    node->elem = elem;
    node->prev = NULL;
    node->next = queue->head;
    if (queue->head == NULL) {
        queue->tail = node;
    } else {
        (queue->head)->prev = node;
    }

    queue->head = node;
    (queue->size)++;
    return 0;
}

void *pop(queue_t *queue) {
    void *tailElem = NULL;
    node_t *tailNode;
    node_t *tailPrevNode;
    if (queue->size > 0) {
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
    }
    return tailElem;
}

void clear(queue_t *queue) {
    node_t *next;
    node_t *curr = queue->head;

    while(curr != NULL) {
        next = curr->next;
        free(curr);
        curr = next;
    }
    queue->size = 0;
}