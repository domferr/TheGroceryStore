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

static int get_store_state(grocerystore_t *gs, gs_state *state);
static int get_cashier_state(cashier_t *ca, cashier_state *state);

void *cashier_fun(void *args) {
    int err, remaining;
    cashier_t *ca = (cashier_t*) args;
    grocerystore_t *gs = ca->gs;
    cashier_state internal_state;
    gs_state store_state;
    client_in_queue *client;

    NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
    NOTZERO(get_cashier_state(ca, &internal_state), perror("get cashier state"); return NULL)
#ifdef DEBUG_CASHIER
    printf("Cassiere %ld: attivo\n", ca->id);
#endif
    while(ISOPEN(store_state)) {
        switch (internal_state) {
            case active:
#ifdef DEBUG_CASHIER
                printf("Cassiere %ld: attendo clienti\n", ca->id);
#endif
                //Prendo il cliente dalla coda. Se non ci sono clienti vengo svegliato se il supermercato deve chiudere
                err = cashier_get_client(ca, &client, &internal_state, &store_state, &remaining);
                NOTZERO(err, perror("cashier remove from queue"); return NULL)
                //Se lo store è ancora aperto e la cassa è ancora aperta allora ho un cliente da servire
                if (ISOPEN(store_state) && internal_state == active) {
#ifdef DEBUG_CASHIER
                    printf("Cassiere %ld: servo un cliente...\n", ca->id);
#endif
                    err = serve_client(ca, client);
                    NOTZERO(err, perror("serve client"); return NULL)
                    err = wakeup_client(client, 1);
                    NOTZERO(err, perror("wakeup client"); return NULL)

                    //Controllo i vari stati per prepararmi alla prossima iterazione
                    NOTZERO(get_cashier_state(ca, &internal_state), perror("get cashier state"); return NULL)
                    NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
                }
                break;
            case sleeping:
                //Sveglia i clienti senza servirli
                NOTZERO(handle_closure(ca, 0), return NULL)
                //Attendo che la cassa sia aperta. Posso essere risvegliato se il supermercato è in chiusura
                err = cashier_wait_activation(ca, gs, &internal_state, &store_state);
                NOTZERO(err, perror("cashier wait activation"); return NULL)
                break;
        }
    }
    //Gestisci chiusura supermercato
    if (store_state == closed) { //Sveglia i clienti e servili tutti
#ifdef DEBUG_CASHIER
        printf("Cassiere %ld: servo tutti i clienti e poi termino\n", ca->id);
#endif
        NOTZERO(handle_closure(ca, 1), return NULL)
    } else if (store_state == closed_fast) { //Sveglia i clienti senza servirli
#ifdef DEBUG_CASHIER
        printf("Cassiere %ld: termino senza servire i clienti\n", ca->id);
#endif
        NOTZERO(handle_closure(ca, 0), return NULL)
    }

#ifdef DEBUG_CASHIER
    printf("Cassiere %ld: termino\n", ca->id);
#endif
    //cleanup
    PTH(err, pthread_mutex_destroy(&(ca->mutex)), return NULL)
    PTH(err, pthread_cond_destroy(&(ca->paused)), return NULL)
    NOTZERO(queue_destroy(ca->queue), perror("cashier queue destroy"); return NULL)
    free(ca);
    return 0;
}

int cashier_wait_activation(cashier_t *ca, grocerystore_t *gs, cashier_state *state, gs_state *store_state) {
    int err;
    NOTZERO(get_store_state(gs, store_state), perror("get store state"); return -1)

    PTH(err, pthread_mutex_lock(&(ca->mutex)), return -1)
    while (ISOPEN(*store_state) && ca->state == sleeping) {
        PTH(err, pthread_cond_wait(&(ca->paused), &(ca->mutex)), return -1)
        PTH(err, pthread_mutex_unlock(&(ca->mutex)), return -1)
        NOTZERO(get_store_state(gs, store_state), perror("get store state"); return -1)
        PTH(err, pthread_mutex_lock(&(ca->mutex)), return -1)
    }
    *store_state = ca->state;
    PTH(err, pthread_mutex_unlock(&(ca->mutex)), return -1)
    return 0;
}

int cashier_get_client(cashier_t *ca, client_in_queue **client, cashier_state *ca_state, gs_state *store_state, int *remaining) {
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
}

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

int handle_closure(cashier_t *ca, int serve) {
    client_in_queue *client;
    queue_t *queue = ca->queue;
    int err, end = 0;
    while (!end) {
        PTH(err, pthread_mutex_lock(&(queue->mtx)), return -1)
        client = pop(queue);
        PTH(err, pthread_mutex_unlock(&(queue->mtx)), return -1)

        if (client != NULL) {
            if (serve) {
                err = serve_client(ca, client);
                NOTZERO(err, perror("serve client"); return -1)
            }
            NOTZERO(wakeup_client(client, serve), return -1)
        } else {
            end = 1;
        }
    }
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