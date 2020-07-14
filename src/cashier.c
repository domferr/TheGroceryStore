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

void *cashier_fun(void *args) {
    int err;
    cashier_t *ca = (cashier_t*) args;
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
        PTH(err, pthread_mutex_lock(&(ca->mutex)), perror("cashier lock"); return NULL)
        switch(ca->state) {
            case sleeping:
#ifdef DEBUG_CASHIER
                printf("Cassiere %ld: chiudo la cassa\n", ca->id);
#endif
                //Sveglia i clienti senza servirli
                while ((ca->queue)->size > 0) {
                    client = pop(ca->queue);
                    wakeup_client(client, cashier_sleeping, stats);
                }
                (stats->closed_counter)++;
                while (ISOPEN(store_state) && ca->state == sleeping) {
                    PTH(err, pthread_cond_wait(&(ca->paused), &(ca->mutex)), perror("cashier cond wait paused"); return NULL)
                    PTH(err, pthread_mutex_unlock(&(ca->mutex)), perror("cashier unlock"); return NULL)
                    NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
                    PTH(err, pthread_mutex_lock(&(ca->mutex)), perror("cashier lock"); return NULL)
                }
                PTH(err, pthread_mutex_unlock(&(ca->mutex)), perror("cashier unlock"); return NULL)
                break;
            case active:
#ifdef DEBUG_CASHIER
                printf("Cassiere %ld: apro la cassa\n", ca->id);
#endif
                while (ISOPEN(store_state) && ca->state == active && (ca->queue)->size == 0) {
                    PTH(err, pthread_cond_wait(&(ca->noclients), &(ca->mutex)), perror("cashier cond wait noclients"); return NULL)
                    PTH(err, pthread_mutex_unlock(&(ca->mutex)), perror("cashier unlock"); return NULL)
                    NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
                    PTH(err, pthread_mutex_lock(&(ca->mutex)), perror("cashier lock"); return NULL)
                }
                client = pop(ca->queue);
                PTH(err, pthread_mutex_unlock(&(ca->mutex)), perror("cashier unlock"); return NULL)

                //Se il cliente è nullo allora la coda è vuota quindi sono stato svegliato perchè
                //il supermercato è in chiusura oppure la cassa è stata chiusa
                if (client != NULL) {
#ifdef DEBUG_CASHIER
                    printf("Cassiere %ld: servo il cliente %ld\n", ca->id, client->id);
#endif
                    NOTZERO(serve_client(ca, client, stats), perror("serve client"); return NULL)
                    NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
                }
                break;
        }
    }


    //Gestisci chiusura supermercato
    MINUS1(handle_closure(ca, store_state, stats), perror("handle closure"); return NULL)

    //cleanup
    PTH(err, pthread_mutex_destroy(&(ca->mutex)), perror("cashier mutex destroy"); return NULL)
    PTH(err, pthread_cond_destroy(&(ca->paused)), perror("cashier cond destroy"); return NULL)
    NOTZERO(queue_destroy(ca->queue), perror("cashier queue destroy"); return NULL)
#ifdef DEBUG_CASHIER
    printf("Cassiere %ld: termino\n", ca->id);
#endif
    free(ca);
    return stats;
}

cashier_t *alloc_cashier(size_t id, grocerystore_t *gs, cashier_state starting_state, int product_service_time) {
    int err;
    unsigned int seed = id;
    cashier_t *ca = (cashier_t*) malloc(sizeof(cashier_t));
    EQNULL(ca, return NULL)

    ca->id = id;
    ca->gs = gs;
    ca->product_service_time = product_service_time;
    ca->fixed_service_time = RANDOM(seed, MIN_SERVICE_TIME, MAX_SERVICE_TIME);  //milliseconds
    ca->state = starting_state;
    ca->queue = queue_create();
    EQNULL(ca->queue, return NULL)

    PTH(err, pthread_mutex_init(&(ca->mutex), NULL), return NULL)
    PTH(err, pthread_cond_init(&(ca->paused), NULL), return NULL)
    PTH(err, pthread_cond_init(&(ca->noclients), NULL), return NULL)

    return ca;
}

int handle_closure(cashier_t *ca, gs_state closing_state, cashier_thread_stats *stats) {
    int err;
    client_in_queue *client;
    PTH(err, pthread_mutex_lock(&(ca->mutex)), return -1)
    if (closing_state == closed) { //Sveglia i clienti e servili tutti
#ifdef DEBUG_CASHIER
        printf("Cassiere %ld: servo tutti i clienti e poi termino\n", ca->id);
#endif
        while ((ca->queue)->size > 0) {
            client = pop(ca->queue);
            NOTZERO(serve_client(ca, client, stats), return -1)
        }
    } else if (closing_state == closed_fast) { //Sveglia i clienti senza servirli
#ifdef DEBUG_CASHIER
        printf("Cassiere %ld: termino senza servire i clienti\n", ca->id);
#endif
        while ((ca->queue)->size > 0) {
            client = pop(ca->queue);
            NOTZERO(wakeup_client(client, store_closure, stats), return -1);
        }
    }
    PTH(err, pthread_mutex_unlock(&(ca->mutex)), return -1)
    return 0;
}

int serve_client(cashier_t *ca, client_in_queue *client, cashier_thread_stats *stats) {
    int err, ms;
    PTH(err, pthread_mutex_lock(&(client->mutex)), return -1)
    ms = ca->fixed_service_time + (ca->product_service_time)*client->products;
    PTH(err, pthread_mutex_unlock(&(client->mutex)), return -1)
#ifdef DEBUG_CASHIER
    printf("Cassiere %ld: Servo un cliente. Impiegherò %dms\n", ca->id, ms);
#endif
    MINUS1(msleep(ms), return -1);
    return wakeup_client(client, done, stats);
}

int wakeup_client(client_in_queue *client, client_status status, cashier_thread_stats *stats) {
    int err;
    PTH(err, pthread_mutex_lock(&(client->mutex)), return -1)
    client->status = status;
    if (status == done) {
        (stats->clients_served)++;
        stats->total_products += client->products;
    }
    PTH(err, pthread_cond_signal(&(client->waiting)), return -1)
    PTH(err, pthread_mutex_unlock(&(client->mutex)), return -1)
    return 0;
}