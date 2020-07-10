#include "cashier.h"
#include "queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

#define NSMULTIPLIER 1000000

void *cashier_fun(void *args) {
    int thread_running = 1;
    struct timespec sleep_value = {0};
    long ms;
    cashier_t *ca = (cashier_t*) args;
    queue_t *queue = ca->queue;
    while(thread_running) {
        //prendo il cliente dalla coda
        client_in_queue *client = (client_in_queue*) removeFromEnd(queue);
        if (client == NULL) return NULL;    //TODO chiamare perror() oppure no?

        pthread_mutex_lock(&(client->mutex));
        ms = ca->service_time + 2*client->products;
        sleep_value.tv_nsec = ms * NSMULTIPLIER;
        pthread_mutex_unlock(&(client->mutex));

        printf("Cassiere: Servo un cliente\n");
        nanosleep(&sleep_value, NULL);

        pthread_mutex_lock(&(client->mutex));
        client->done = 1;
        pthread_cond_signal(&(client->waiting));
        pthread_mutex_unlock(&(client->mutex));

        //TODO cosa fare se non ci sono clienti in coda ma devi comunque terminare
        pthread_mutex_lock(&(ca->mutex));
        while (ca->state == PAUSE) {
            pthread_cond_wait(&(ca->paused), &(ca->mutex));
        }
        thread_running = ca->state != STOP;
        pthread_mutex_unlock(&(ca->mutex));
    }

    return 0;
}

void run_cashier(cashier_t *ca) {
    pthread_mutex_lock(&(ca->mutex));
    ca->state = RUN;
    pthread_cond_signal(&(ca->paused));
    pthread_mutex_unlock(&(ca->mutex));
}

void pause_cashier(cashier_t *ca) {
    pthread_mutex_lock(&(ca->mutex));
    ca->state = PAUSE;
    pthread_mutex_unlock(&(ca->mutex));
}

void stop_cashier(cashier_t *ca) {
    pthread_mutex_lock(&(ca->mutex));
    ca->state = STOP;
    pthread_mutex_unlock(&(ca->mutex));
}

cashier_t *alloc_cashier(long service_time) {
    cashier_t *ca = (cashier_t*) malloc(sizeof(cashier_t));
    if (ca == NULL) return NULL;

    ca->service_time = service_time;
    ca->state = RUN;
    ca->queue = queue_create();
    if (ca->queue == NULL) {
        free(ca);
        return NULL;
    }
    if (pthread_mutex_init(&(ca->mutex), NULL) != 0) {
        queue_destroy(ca->queue);
        free(ca);
        return NULL;
    }
    if (pthread_cond_init(&(ca->paused), NULL) != 0) {
        pthread_mutex_destroy(&(ca->mutex));
        queue_destroy(ca->queue);
        free(ca);
        return NULL;
    }

    return ca;
}

void free_cashier(cashier_t *ca) {
    pthread_mutex_destroy(&(ca->mutex));
    pthread_cond_destroy(&(ca->paused));
    queue_destroy(ca->queue);
    free(ca);
}