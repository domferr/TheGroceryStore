#include "utils.h"
#include "grocerystore.h"
#include "cashier.h"
#include "client_in_queue.h"
#include "queue.h"
#include "logger.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

#define DEBUG_CASHIER
#define MIN_SERVICE_TIME 20 //ms
#define MAX_SERVICE_TIME 80 //ms

static int push_activation_time(cashier_thread_stats *stats, struct timespec *active_start);

void *cashier_fun(void *args) {
    int err;
    struct timespec active_start = {0,0};
    cashier_t *ca = (cashier_t*) args;
    cashier_sync *ca_sync = ca->ca_sync;
    pthread_mutex_t *mutex = &(ca_sync->mutex);
    pthread_cond_t *paused = &(ca_sync->paused);
    pthread_cond_t *noclients = &(ca_sync->noclients);
    grocerystore_t *gs = ca->gs;
    gs_state store_state;
    client_in_queue *client;
#ifdef DEBUG_CASHIER
    printf("Cassiere %ld: attivo\n", ca->id);
#endif
    cashier_thread_stats *stats = alloc_cashier_thread_stats(ca->id);
    EQNULL(stats, perror("alloc cashier thread stats"); return NULL)

    NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
    while(ISOPEN(store_state)) {
        PTH(err, pthread_mutex_lock(mutex), perror("cashier lock"); return NULL)
        switch(ca_sync->state) {
            case sleeping:
                //Sveglia i clienti senza servirli
                while ((ca->queue)->size > 0) {
                    client = pop(ca->queue);
                    wakeup_client(client, cashier_sleeping);
                }
                while (ISOPEN(store_state) && ca_sync->state == sleeping) {
                    PTH(err, pthread_cond_wait(paused, mutex), perror("cashier cond wait paused"); return NULL)
                    PTH(err, pthread_mutex_unlock(mutex), perror("cashier unlock"); return NULL)
                    NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
                    PTH(err, pthread_mutex_lock(mutex), perror("cashier lock"); return NULL)
                }
                if (ca_sync->state == active) {
#ifdef DEBUG_CASHIER
                    printf("Cassiere %ld: apro la cassa\n", ca->id);
#endif
                    MINUS1(clock_gettime(CLOCK_MONOTONIC, &active_start), perror("clock_gettime"); return NULL)
                }
                PTH(err, pthread_mutex_unlock(mutex), perror("cashier unlock"); return NULL)
                break;
            case active:
                //Se non ho mai preso il tempo di attivazione da quando la cassa è stata attivata
                if (active_start.tv_nsec == 0) {
                    MINUS1(clock_gettime(CLOCK_MONOTONIC, &active_start), perror("clock_gettime"); return NULL)
                }
                while (ISOPEN(store_state) && ca_sync->state == active && (ca->queue)->size == 0) {
                    PTH(err, pthread_cond_wait(noclients, mutex), perror("cashier cond wait noclients"); return NULL)
                    PTH(err, pthread_mutex_unlock(mutex), perror("cashier unlock"); return NULL)
                    NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
                    PTH(err, pthread_mutex_lock(mutex), perror("cashier lock"); return NULL)
                }
                client = pop(ca->queue);
                PTH(err, pthread_mutex_unlock(mutex), perror("cashier unlock"); return NULL)

                //Se il cliente è nullo allora la coda è vuota quindi sono stato svegliato perchè
                //il supermercato è in chiusura oppure la cassa è stata chiusa
                if (client != NULL) {
#ifdef DEBUG_CASHIER
                    printf("Cassiere %ld: servo il cliente %ld\n", ca->id, client->id);
#endif
                    err = serve_client(ca->id, ca->fixed_service_time, ca->product_service_time, client, stats);
                    NOTZERO(err, perror("serve client"); return NULL)
                    NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
                }
                PTH(err, pthread_mutex_lock(mutex), perror("cashier unlock"); return NULL)
                if (ca_sync->state == sleeping) {  //la cassa è stata chiusa
                    (stats->closed_counter)++;
                    MINUS1(push_activation_time(stats, &active_start), perror("push activation time"); return NULL)
#ifdef DEBUG_CASHIER
                    printf("Cassiere %ld: chiudo la cassa\n", ca->id);
#endif
                }
                PTH(err, pthread_mutex_unlock(mutex), perror("cashier unlock"); return NULL)
                break;
        }
    }

    //Gestisci chiusura supermercato
    MINUS1(handle_closure(ca, store_state, stats), perror("handle closure"); return NULL)

    //cleanup
    PTH(err, pthread_mutex_destroy(mutex), perror("cashier mutex destroy"); return NULL)
    PTH(err, pthread_cond_destroy(paused), perror("cashier cond destroy"); return NULL)
    NOTZERO(queue_destroy(ca->queue), perror("cashier queue destroy"); return NULL)
    free(ca_sync);
    free(ca);
    return stats;
}

cashier_t *alloc_cashier(size_t id, grocerystore_t *gs, cashier_state starting_state, int product_service_time) {
    int err;
    unsigned int seed = id;
    cashier_t *ca = (cashier_t*) malloc(sizeof(cashier_t));
    EQNULL(ca, return NULL)
    cashier_sync *ca_sync = (cashier_sync*) malloc(sizeof(cashier_sync));
    EQNULL(ca_sync, return NULL)
    ca->id = id;
    ca->gs = gs;
    ca->product_service_time = product_service_time;
    ca->fixed_service_time = RANDOM(seed, MIN_SERVICE_TIME, MAX_SERVICE_TIME);  //milliseconds
    ca->queue = queue_create();
    EQNULL(ca->queue, return NULL)

    ca_sync->state = starting_state;
    PTH(err, pthread_mutex_init(&(ca_sync->mutex), NULL), return NULL)
    PTH(err, pthread_cond_init(&(ca_sync->paused), NULL), return NULL)
    PTH(err, pthread_cond_init(&(ca_sync->noclients), NULL), return NULL)
    ca->ca_sync = ca_sync;
    return ca;
}

int handle_closure(cashier_t *ca, gs_state closing_state, cashier_thread_stats *stats) {
    int err;
    client_in_queue *client;
    cashier_sync *ca_sync = ca->ca_sync;
    queue_t *queue = ca->queue;
    PTH(err, pthread_mutex_lock(&(ca_sync->mutex)), return -1)
    if (ca_sync->state == active && closing_state == closed) { //Sveglia i clienti e servili tutti
#ifdef DEBUG_CASHIER
        printf("Cassiere %ld: servo tutti i clienti e poi termino\n", ca->id);
#endif
        while (queue->size > 0) {
            client = pop(queue);
            NOTZERO(serve_client(ca->id, ca->fixed_service_time, ca->product_service_time, client, stats), return -1)
        }
    } else if (ca_sync->state == active && closing_state == closed_fast) { //Sveglia i clienti senza servirli
#ifdef DEBUG_CASHIER
        printf("Cassiere %ld: termino senza servire i clienti\n", ca->id);
#endif
        while (queue->size > 0) {
            client = pop(queue);
            NOTZERO(wakeup_client(client, store_closure), return -1);
        }
    }
    PTH(err, pthread_mutex_unlock(&(ca_sync->mutex)), return -1)
    return 0;
}

static int push_activation_time(cashier_thread_stats *stats, struct timespec *active_start) {
    int *ptr;
    struct timespec active_end;
    MINUS1(clock_gettime(CLOCK_MONOTONIC, &active_end), return -1)
    EQNULL((ptr = (int*) malloc(sizeof(int))), return -1)
    *ptr = get_elapsed_milliseconds(*active_start, active_end);
    MINUS1(push(stats->active_times, ptr), return -1);
    active_start->tv_sec = 0;
    active_start->tv_nsec = 0;
    return 0;
}

int serve_client(size_t id, int fixed_time, int product_time, client_in_queue *client, cashier_thread_stats *stats) {
    int err, *ptr, ms;
    PTH(err, pthread_mutex_lock(&(client->mutex)), return -1)
    ms = fixed_time + (product_time*(client->products));
    PTH(err, pthread_mutex_unlock(&(client->mutex)), return -1)
#ifdef DEBUG_CASHIER
    printf("Cassiere %ld: Servo un cliente. Impiegherò %dms\n", id, ms);
#endif
    MINUS1(msleep(ms), return -1);
    //Aggiorno le statistiche
    EQNULL((ptr = (int*) malloc(sizeof(int))), return -1)
    *ptr = ms;
    push(stats->clients_times, ptr);
    stats->total_products += client->products;
    //Sveglio il cliente
    return wakeup_client(client, done);
}

int wakeup_client(client_in_queue *client, client_status status) {
    int err;

    PTH(err, pthread_mutex_lock(&(client->mutex)), return -1)
    client->status = status;
    PTH(err, pthread_cond_signal(&(client->waiting)), return -1)
    PTH(err, pthread_mutex_unlock(&(client->mutex)), return -1)
    return 0;
}