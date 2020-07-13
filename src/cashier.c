#include "utils.h"
#include "grocerystore.h"
#include "cashier.h"
#include "queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

//#define DEBUG_CASHIER
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
                    wakeup_client(client, 0);
                }
                while (ISOPEN(store_state) && ca->state == sleeping) {
                    PTH(err, pthread_cond_wait(&(ca->paused), &(ca->mutex)), perror("cashier cond wait paused"); return NULL)
                    PTH(err, pthread_mutex_unlock(&(ca->mutex)), perror("cashier unlock"); return NULL)
                    NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
                    PTH(err, pthread_mutex_lock(&(ca->mutex)), perror("cashier lock"); return NULL)
                }
                PTH(err, pthread_mutex_unlock(&(ca->mutex)), perror("cashier unlock"); return NULL)
                break;
            case active:
                while (ISOPEN(store_state) && ca->state == active && (ca->queue)->size == 0) {
                    PTH(err, pthread_cond_wait(&(ca->noclients), &(ca->mutex)), perror("cashier cond wait noclients"); return NULL)
                    PTH(err, pthread_mutex_unlock(&(ca->mutex)), perror("cashier unlock"); return NULL)
                    NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
                    PTH(err, pthread_mutex_lock(&(ca->mutex)), perror("cashier lock"); return NULL)
                }
                client = pop(ca->queue);
                PTH(err, pthread_mutex_unlock(&(ca->mutex)), perror("cashier unlock"); return NULL)

                if (client != NULL) {   //Ho preso un cliente dalla coda
#ifdef DEBUG_CASHIER
                    printf("Cassiere %ld: servo il prossimo cliente\n", ca->id);
#endif
                    err = serve_client(ca, client);
                    NOTZERO(err, perror("serve client"); return NULL)
                    err = wakeup_client(client, 1);
                    NOTZERO(err, perror("wake up client"); return NULL)
                }
                //Se il cliente è nullo allora la coda è vuota quindi sono stato svegliato perchè
                //il supermercato è in chiusura oppure la cassa è stata chiusa
                break;
        }
    }

    PTH(err, pthread_mutex_lock(&(ca->mutex)), perror("cashier unlock"); return NULL)
    //Gestisci chiusura supermercato
    if (store_state == closed) { //Sveglia i clienti e servili tutti
#ifdef DEBUG_CASHIER
        printf("Cassiere %ld: servo tutti i clienti e poi termino\n", ca->id);
#endif
        while ((ca->queue)->size > 0) {
            client = pop(ca->queue);
            err = serve_client(ca, client);
            NOTZERO(err, perror("serve client"); return NULL)
            err = wakeup_client(client, 1);
            NOTZERO(err, perror("wake up client"); return NULL)
        }
    } else if (store_state == closed_fast) { //Sveglia i clienti senza servirli
#ifdef DEBUG_CASHIER
        printf("Cassiere %ld: termino senza servire i clienti\n", ca->id);
#endif
        while ((ca->queue)->size > 0) {
            client = pop(ca->queue);
            NOTZERO(wakeup_client(client, 0), perror("wake up client"); return NULL);
        }
    }
    PTH(err, pthread_mutex_unlock(&(ca->mutex)), perror("cashier unlock"); return NULL)

    //cleanup
    PTH(err, pthread_mutex_destroy(&(ca->mutex)), perror("cashier mutex destroy"); return NULL)
    PTH(err, pthread_cond_destroy(&(ca->paused)), perror("cashier cond destroy"); return NULL)
    NOTZERO(queue_destroy(ca->queue), perror("cashier queue destroy"); return NULL)
    free(ca);
#ifdef DEBUG_CASHIER
    printf("Cassiere %ld: termino\n", ca->id);
#endif
    return 0;
}

/*int cashier_get_client(cashier_t *ca, client_in_queue **client, gs_state *store_state) {
    int err;
    void *popped;
    queue_t *queue = ca->queue;
    grocerystore_t *gs = ca->gs;
    NOTZERO(get_store_state(gs, store_state), perror("get store state"); return -1)

    PTH(err, pthread_mutex_lock(&(queue->mtx)), return -1)
    //Se lo store è aperto, se la cassa è ancora aperta e se la coda è vuota, aspetto
    while(ISOPEN(*store_state) && *ca_state == active && queue->size == 0) {
        PTH(err, pthread_cond_wait(&(queue->empty), &(queue->mtx)), return -1)
        PTH(err, pthread_mutex_unlock(&(queue->mtx)), return -1)
        //se vengo risvegliato posso riaddormentarmi solo se lo store è ancora aperto
        printf("WHILE DEL CLIENTE\n");
        NOTZERO(get_store_state(gs, store_state), perror("get store state"); return -1)
        NOTZERO(get_cashier_state(ca, ca_state), perror("get cashier state"); return -1)
        PTH(err, pthread_mutex_lock(&(queue->mtx)), return -1)
    }
    //Prendo il cliente solo se il supermercato è aperto, se la cassa è attiva e se c'è almeno un cliente in coda
    if (ISOPEN(*store_state) && *ca_state == active && queue->size > 0) {
        EQNULL((popped = pop(queue)), return -1)
        *client = (client_in_queue*) popped;
    }

    *remaining = queue->size;
    PTH(err, pthread_mutex_unlock(&(queue->mtx)), return -1)

    return 0;
}*/

int serve_client(cashier_t *ca, client_in_queue *client) {
    int err, ms;
    PTH(err, pthread_mutex_lock(&(client->mutex)), return -1)
    ms = ca->fixed_service_time + (ca->product_service_time)*client->products;
    PTH(err, pthread_mutex_unlock(&(client->mutex)), return -1)
#ifdef DEBUG_CASHIER
    printf("Cassiere %ld: Servo un cliente. Impiegherò %dms\n", ca->id, ms);
#endif
    return msleep(ms);
}

int wakeup_client(client_in_queue *client, int done) {
    int err;
    PTH(err, pthread_mutex_lock(&(client->mutex)), return -1)
    client->done = done;
    PTH(err, pthread_cond_signal(&(client->waiting)), return -1)
    PTH(err, pthread_mutex_unlock(&(client->mutex)), return -1)
    return 0;
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