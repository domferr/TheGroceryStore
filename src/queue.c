//
// Created by Domenico on 07/07/2020.
//

#include "queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

static void recursiveToNext(node_t *node, void (*jobFun)(void*));
static void recursiveToPrev(node_t *node, void (*jobFun)(void*));

queue_t *queue_create() {
    queue_t *queue = (queue_t*) malloc(sizeof(queue_t));
    if (queue == NULL) {
        return NULL;
    }
    queue->size = 0;
    queue->head = NULL;
    queue->tail = NULL;
    if (pthread_mutex_init(&(queue->mtx), NULL) != 0) {
        free(queue);
        return NULL;
    }

    if (pthread_cond_init(&(queue->empty), NULL) != 0) {
        pthread_mutex_destroy(&(queue->mtx));
        free(queue);
        return NULL;
    }
    return queue;
}

int addAtStart(queue_t *queue, void *elem) {
    node_t *node = (node_t*) malloc(sizeof(node_t));
    if (node == NULL)
        return -1;
    node->elem = elem;
    node->prev = NULL;

    pthread_mutex_lock(&(queue->mtx));
    node->next = queue->head;
    if (queue->head == NULL) {
        queue->tail = node;
    } else {
        (queue->head)->prev = node;
    }

    queue->head = node;
    (queue->size)++;
    pthread_mutex_unlock(&(queue->mtx));
    return 0;
}

void fromFirst(queue_t *queue, void (*jobFun)(void*)) {
    pthread_mutex_lock(&(queue->mtx));
    recursiveToNext(queue->head, jobFun);
    pthread_mutex_unlock(&(queue->mtx));
}

void fromLast(queue_t *queue, void (*jobFun)(void*)) {
    pthread_mutex_lock(&(queue->mtx));
    recursiveToPrev(queue->tail, jobFun);
    pthread_mutex_unlock(&(queue->mtx));
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