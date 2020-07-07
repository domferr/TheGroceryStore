//
// Created by Domenico on 07/07/2020.
//

#include "queue.h"
#include <stdlib.h>

int queue_create(queue_t *queue) {
    queue = (queue_t*) malloc(sizeof(queue_t));
    if (queue == NULL) {
        return -1;
    }
    queue->size = 0;
    queue->head = NULL;
    queue->tail = NULL;
    if (pthread_mutex_init(&(queue->mtx), NULL) != 0) {
        free(queue);
        return -1;
    }

    if (pthread_cond_init(&(queue->empty), NULL) != 0) {
        pthread_mutex_destroy(&(queue->mtx));
        free(queue);
        return -1;
    }
    return 0;
}