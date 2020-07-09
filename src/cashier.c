#define _POSIX_C_SOURCE 199309L
#include "cashier.h"
#include "queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

#define NSMULTIPLIER 1000000

static void doService(int products, long service_time);
static void removeAllClients(queue_t *queue);

void *cashier(void *args) {
    int thread_running = 1;
    cashier_t *ca = (cashier_t*) args;
    queue_t *queue = ca->queue;
    while(thread_running) {
        client_in_queue *client = (client_in_queue*) removeFromEnd(queue);
        if (client != NULL) {
            printf("Cassiere: Servo un cliente\n");
            doService(client->products, ca->service_time);
        }

        //TODO cosa fare se non ci sono clienti in coda ma devi comunque terminare
        pthread_mutex_lock(&(ca->mutex));
        if (ca->state == PAUSE)
            removeAllClients(queue);
        while (ca->state == PAUSE) {
            pthread_cond_wait(&(ca->paused), &(ca->mutex));
        }
        if (ca->state == STOP) {
            thread_running = 0;
        } else if (ca->state == RUN) {
            thread_running = 1;
        }
        pthread_mutex_unlock(&(ca->mutex));
    }

    return 0;
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

static void doService(int products, long service_time) {
    struct timespec sleep_value = {0};
    long ms = service_time + (long)(2*products);
    sleep_value.tv_nsec = ms * NSMULTIPLIER;
    nanosleep(&sleep_value, NULL);
}

static void removeAllClients(queue_t *queue) {

}