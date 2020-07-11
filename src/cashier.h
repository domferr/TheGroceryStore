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
    pthread_mutex_t mutex;
    pthread_cond_t waiting;
    int done;
} client_in_queue;

void *cashier_fun(void *args);
cashier_t *alloc_cashier(long service_time);
void cashier_free(cashier_t *ca);
void run_cashier(cashier_t *ca);
void pause_cashier(cashier_t *ca);
void stop_cashier(cashier_t *ca);

#endif //CASHIER_H
