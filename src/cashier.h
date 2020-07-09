#ifndef CASHIER_H
#define CASHIER_H

#include "queue.h"
#include <pthread.h>

enum state {
    RUN, //continua a servire i clienti in coda
    PAUSE, //servi il cliente e rimani in attesa di essere riattivato
    STOP //servi il cliente e termina
};

typedef struct {
    queue_t *queue;
    long service_time;  //milliseconds
    enum state state;
    pthread_cond_t paused;
    pthread_mutex_t mutex;
} cashier_t;

typedef struct {
    int products;
    pthread_cond_t waiting;
    pthread_mutex_t mutex;
} client_in_queue;

void *cashier(void *args);
cashier_t *alloc_cashier(long service_time);
void free_cashier(cashier_t *ca);

#endif //CASHIER_H
