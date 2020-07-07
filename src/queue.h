#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

typedef struct node_t {
    void *elem;
    struct node_t *next;
    struct node_t *prev;
} node_t;

typedef struct {
    pthread_mutex_t mtx;
    node_t *head;
    node_t *tail;
    int size;  //Quanti elementi ci sono nella lista
    pthread_cond_t empty;
} queue_t;

queue_t *queue_create();
int addAtStart(queue_t *queue, void *elem);
void fromFirst(queue_t *queue, void (*jobFun)(void*));
void fromLast(queue_t *queue, void (*jobFun)(void*));

#endif //QUEUE_H
