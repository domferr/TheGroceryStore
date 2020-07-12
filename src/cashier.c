#include "utils.h"
#include "grocerystore.h"
#include "cashier.h"
#include "queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

#define DEBUG_CASHIER
#define MIN_SERVICE_TIME 20 //ms
#define MAX_SERVICE_TIME 80 //ms

static int get_store_state(grocerystore_t *gs, gs_state *state);
static int get_cashier_state(cashier_t *ca, cashier_state *state);

void *cashier_fun(void *args) {
    int err, fixed_service_time;
    cashier_t *ca = (cashier_t*) args;
    grocerystore_t *gs = ca->gs;
    unsigned int seed = ca->id;
    cashier_state internal_state;
    gs_state store_state;
    client_in_queue *client;

    //TODO spostare fixed_service_time nella struct cashier_t oppure lasciarlo qui
    fixed_service_time = RANDOM(seed, MIN_SERVICE_TIME, MAX_SERVICE_TIME);  //milliseconds
    NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
    NOTZERO(get_cashier_state(ca, &internal_state), perror("get cashier state"); return NULL)

    while(ISOPEN(store_state)) {
        switch (internal_state) {
            case active:
                //Prendo il cliente dalla coda. Se non ci sono clienti vengo svegliato se il supermercato deve chiudere
                err = cashier_get_client(ca, &client, &internal_state);
                NOTZERO(err, perror("cashier remove from queue"); return NULL)
                //Dopo questa attesa, prendo il nuovo stato del supermercato
                NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
                //Se lo store è ancora aperto e la cassa è ancora aperta
                if (ISOPEN(store_state) && internal_state == active) {
                    serve_client(ca, client, &internal_state, fixed_service_time);
                }
                break;
            case sleeping:
                //Attendo che la cassa sia aperta
                err = cashier_wait_activation(ca, gs, &internal_state);
                NOTZERO(err, perror("cashier wait activation"); return NULL)
                break;
        }
        //Prima di iterare ancora, prendo lo stato del supermercato
        NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
    }
    //Gestisci chiusura supermercato
    if (store_state == closed) {
        //Sveglia i clienti e servili tutti

    } else if (store_state == closed_fast) {
        //Sveglia i clienti senza servirli
    }

    printf("Cassiere %ld: termino\n", ca->id);
    //cleanup
    PTH(err, pthread_mutex_destroy(&(ca->mutex)), return NULL)
    PTH(err, pthread_cond_destroy(&(ca->paused)), return NULL)
    NOTZERO(queue_destroy(ca->queue), perror("cashier queue destroy"); return NULL)
    free(ca);
    return 0;
}

int cashier_wait_activation(cashier_t *ca, grocerystore_t *gs, cashier_state *state) {
    int err;
    gs_state store_state;
    NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return -1)

    PTH(err, pthread_mutex_lock(&(ca->mutex)), return -1)
    while (ISOPEN(store_state) && ca->state == sleeping) {
        PTH(err, pthread_cond_wait(&(ca->paused), &(ca->mutex)), return -1)
        PTH(err, pthread_mutex_unlock(&(ca->mutex)), return -1)
        NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return -1)
        PTH(err, pthread_mutex_lock(&(ca->mutex)), return -1)
    }
    *state = ca->state;
    PTH(err, pthread_mutex_unlock(&(ca->mutex)), return -1)
    return 0;
}

int cashier_get_client(cashier_t *ca, client_in_queue **client, cashier_state *ca_state) {
    int err;
    queue_t *queue = ca->queue;
    grocerystore_t *gs = ca->gs;
    gs_state store_state;
    NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return -1)

    PTH(err, pthread_mutex_lock(&(queue->mtx)), return -1)
    //Se lo store è aperto, se la cassa è ancora aperta e se la coda è vuota, aspetto
    while(ISOPEN(store_state) && *ca_state == active && queue->size == 0) {
        PTH(err, pthread_cond_wait(&(queue->empty), &(queue->mtx)), return -1)
        PTH(err, pthread_mutex_unlock(&(queue->mtx)), return -1)
        //se vengo risvegliato posso riaddormentarmi solo se lo store è ancora aperto
        NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return -1)
        NOTZERO(get_cashier_state(ca, ca_state), perror("get cashier state"); return -1)
        PTH(err, pthread_mutex_lock(&(queue->mtx)), return -1)
    }
    PTH(err, pthread_mutex_unlock(&(queue->mtx)), return -1)

    return 0;
}

int serve_client(cashier_t *ca, client_in_queue *client, cashier_state *state, int fixed_service_time) {
    int err, ms;
    PTH(err, pthread_mutex_lock(&(client->mutex)), return -1)
    ms = fixed_service_time + (ca->product_service_time)*client->products;
    PTH(err, pthread_mutex_unlock(&(client->mutex)), return -1)
    printf("Cassiere %ld: Servo un cliente. Impiegherò %dms\n", ca->id, ms);
    NOTZERO(msleep(ms), return -1);
    wake_client(client, 1);
    return 0;
}

void wake_client(client_in_queue *client, int done) {
    pthread_mutex_lock(&(client->mutex));
    client->done = done;
    pthread_cond_signal(&(client->waiting));
    pthread_mutex_unlock(&(client->mutex));
}

cashier_t *alloc_cashier(size_t id, grocerystore_t *gs, cashier_state starting_state, int product_service_time) {
    int err;
    cashier_t *ca = (cashier_t*) malloc(sizeof(cashier_t));
    EQNULL(ca, return NULL)

    ca->id = id;
    ca->gs = gs;
    ca->product_service_time = product_service_time;
    ca->state = starting_state;
    ca->queue = queue_create();
    EQNULL(ca->queue, return NULL)

    PTH(err, pthread_mutex_init(&(ca->mutex), NULL), return NULL)
    PTH(err, pthread_cond_init(&(ca->paused), NULL), return NULL)

    return ca;
}

static int get_store_state(grocerystore_t *gs, gs_state *state) {
    int err;
    PTH(err, pthread_mutex_lock(&(gs->mutex)), return -1)
    *state = gs->state;
    PTH(err, pthread_mutex_unlock(&(gs->mutex)), return -1)
    return 0;
}

static int get_cashier_state(cashier_t *ca, cashier_state *state) {
    int err;
    PTH(err, pthread_mutex_lock(&(ca->mutex)), return -1)
    *state = ca->state;
    PTH(err, pthread_mutex_unlock(&(ca->mutex)), return -1)
    return 0;
}