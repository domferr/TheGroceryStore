#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

typedef struct {
    void *value;
    struct threads *next;
    struct threads *prev;
} node_t;

typedef struct {
    pthread_mutex_t mtx;
    node_t *head;
    node_t *tail;
    int size;  //Quanti elementi ci sono nella lista
    pthread_cond_t empty;
} queue_t;

int queue_create(queue_t *queue);

#endif //QUEUE_H
