/**
 * Implementazione di una unbounded queue FIFO.
 */

#include "../include/queue.h"
#include "../include/utils.h"
#include <stdlib.h>

static int internal_foreach(node_t *node, int (*fun)(void*, void*), void *args);

queue_t *queue_create(void) {
    queue_t *queue = (queue_t*) malloc(sizeof(queue_t));
    EQNULL(queue, return NULL)
    queue->size = 0;
    queue->head = NULL;
    queue->tail = NULL;
    return queue;
}

int queue_destroy(queue_t *queue, void (*free_fun)(void *elem)) {
    clear(queue, free_fun);
    free(queue);
    return 0;
}

int push(queue_t *queue, void *elem) {
    node_t *node = (node_t *) malloc(sizeof(node_t));
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
    queue->size++;
    return 0;
}

static void *remove_n(queue_t *queue, node_t *node) {
    void *elem = NULL;
    node_t *prev_node;
    node_t *next_node;
    if (node != NULL) {
        elem = node->elem;
        prev_node = node->prev;
        next_node = node->next;
        if (queue->head == node)
            queue->head = next_node;
        if (queue->tail == node)
            queue->tail = prev_node;
        if (prev_node != NULL)
            prev_node->next = next_node;
        if (next_node != NULL)
            next_node->prev = prev_node;
        queue->size--;
        free(node);
    }
    return elem;
}

void *pop(queue_t *queue) {
    return remove_n(queue, queue->tail);
}

void clear(queue_t *queue, void (*free_fun)(void *elem)) {
    void *elem;
    while(queue->size > 0) {
        elem = pop(queue);
        if (free_fun != NULL)
            free_fun(elem);
    }
}

int foreach(queue_t *queue, int (*fun)(void*, void*), void *args) {
    return internal_foreach(queue->tail, fun, args);
}

static int internal_foreach(node_t *node, int (*fun)(void*, void*), void *args) {
    if (node != NULL) {
        MINUS1(fun(node->elem, args), return -1)
        return internal_foreach(node->prev, fun, args);
    }
    return 0;
}

void merge(queue_t *q1, queue_t *q2) {
    if (q1 && q2) {
        q1->size += q2->size;
        if (q1->tail)
            q1->tail->next = q2->head;
        if (q2->head)
            q2->head->prev = q1->tail;

        q1->tail = q2->tail;
        free(q2);
    }
}