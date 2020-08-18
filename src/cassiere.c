#define DEBUGGING 0
#include "../include/utils.h"
#include "../include/cassiere.h"
#include "../include/store.h"
#include "../include/notifier.h"
#include "../include/client_in_queue.h"
#include "../include/cassa_queue.h"
#include <stdlib.h>
#include <stdio.h>

#define MIN_SERVICE_TIME 20 //ms
#define MAX_SERVICE_TIME 80 //ms

//TODO fare questa documentazione
static int serve_client(cassiere_t *ca, client_in_queue_t *client);

cassiere_t *alloc_cassiere(size_t id, store_t *store, int isopen, int product_service_time, int interval) {
    int err;
    unsigned int seed = id;

    cassiere_t *cassiere = (cassiere_t*) malloc(sizeof(cassiere_t));
    EQNULL(cassiere, return NULL)

    cassiere->id = id;
    cassiere->isopen = isopen;
    cassiere->store = store;
    cassiere->product_service_time = product_service_time;
    cassiere->fixed_service_time = RANDOM(seed, MIN_SERVICE_TIME, MAX_SERVICE_TIME);  //milliseconds
    cassiere->interval = interval;
    EQNULL(cassiere->queue = cassa_queue_create(), return NULL)
    PTH(err, pthread_mutex_init(&(cassiere->mutex), NULL), return NULL)
    PTH(err, pthread_cond_init(&(cassiere->noclients), NULL), return NULL)
    PTH(err, pthread_cond_init(&(cassiere->waiting), NULL), return NULL)

    return cassiere;
}

int cassiere_destroy(cassiere_t *cassiere) {
    int err;
    MINUS1(cassa_queue_destroy(cassiere->queue), return -1)
    PTH(err, pthread_mutex_destroy(&(cassiere->mutex)), return -1)
    PTH(err, pthread_cond_destroy(&(cassiere->noclients)), return -1)
    PTH(err, pthread_cond_destroy(&(cassiere->waiting)), return -1)
    free(cassiere);
    return 0;
}

void *cassiere_thread_fun(void *args) {
    cassiere_t *ca = (cassiere_t*) args;
    int err;
    client_in_queue_t *client;
    struct timespec open_start;
    pthread_t th_notifier;
    store_state st_state;
    notifier_t *notifier = alloc_notifier(ca, ca->isopen);
    EQNULL(notifier, perror("alloc notifier"); exit(EXIT_FAILURE))
    PTH(err, pthread_create(&th_notifier, NULL, &notifier_thread_fun, notifier), perror("create notifier thread"); return NULL)

    NOTZERO(get_store_state(&st_state), perror("get store state"); return NULL)
    while(ISOPEN(st_state)) {
        PTH(err, pthread_mutex_lock(&(ca->mutex)), perror("lock"); return NULL)
        if (ca->isopen) {
            MINUS1(set_notifier_state(notifier, notifier_run), perror("set notifier state"); return NULL)
            //esco se il supermercato viene chiuso o se la cassa viene chiusa o se arriva un cliente
            while (ISOPEN(st_state) && ca->isopen && ca->queue->size == 0) {
                PTH(err, pthread_cond_wait(&(ca->noclients), &(ca->mutex)), perror("cond wait"); return NULL)
                PTH(err, pthread_mutex_unlock(&(ca->mutex)), perror("unlock"); return NULL)
                NOTZERO(get_store_state(&st_state), perror("get store state"); return NULL)
                PTH(err, pthread_mutex_lock(&(ca->mutex)), perror("lock"); return NULL)
            }
            //C'è un cliente e sia la cassa che il supermercato sono aperti
            if (ISOPEN(st_state) && ca->isopen) {
                client = get_next_client(ca, 1);
                PTH(err, pthread_mutex_unlock(&(ca->mutex)), perror("unlock"); return NULL)
                MINUS1(serve_client(ca, client), perror("serve client"); return NULL)
                PTH(err, pthread_mutex_lock(&(ca->mutex)), perror("lock"); return NULL)
                //Sveglio il cliente
                MINUS1(wakeup_client(client, 1), perror("wake up client"); return NULL)
            }
        } else {
            DEBUG("Cassiere %ld: cassa chiusa, clienti in coda %d\n", ca->id, ca->queue->size)
            //Avvisa tutti i clienti in coda che la cassa è chiusa
            while(ca->queue->size > 0) {
                client = get_next_client(ca, 0);
                MINUS1(wakeup_client(client, 0), perror("wake up client"); return NULL)
            }
            DEBUG("Cassiere %ld: cassa chiusa, clienti in coda %d DOPO\n", ca->id, ca->queue->size)
            //Aspetto fino a quando il supermercato chiude o quando il direttore apre la cassa
            while (ISOPEN(st_state) && !ca->isopen) {
                PTH(err, pthread_cond_wait(&(ca->waiting), &(ca->mutex)), perror("cond wait"); return NULL)
                PTH(err, pthread_mutex_unlock(&(ca->mutex)), perror("unlock"); return NULL)
                NOTZERO(get_store_state(&st_state), perror("get store state"); return NULL)
                PTH(err, pthread_mutex_lock(&(ca->mutex)), perror("lock"); return NULL)
            }
            if (ISOPEN(st_state) && ca->isopen)
                MINUS1(clock_gettime(CLOCK_MONOTONIC, &open_start), perror("clock_gettime"); return NULL)
        }
        PTH(err, pthread_mutex_unlock(&(ca->mutex)), perror("unlock"); return NULL)
        NOTZERO(get_store_state(&st_state), perror("get store state"); return NULL)
    }

    DEBUG("Cassiere %ld: faccio terminare il notifier\n", ca->id)
    MINUS1(set_notifier_state(notifier, notifier_quit), perror("set notifier state"); return NULL)
    PTH(err, pthread_join(th_notifier, NULL), perror("join notifier"); return NULL)
    MINUS1(destroy_notifier(notifier), perror("destroy notifier"); return NULL)
    DEBUG("Cassiere %ld: gestisco clienti rimasti in coda\n", ca->id)
    PTH(err, pthread_mutex_lock(&(ca->mutex)), perror("lock"); return NULL)
    while(ca->queue->size > 0) {
        DEBUG("size: %d - %p\n", ca->queue->size, (void*)ca->queue->head)
        client = get_next_client(ca, st_state != closed_fast_state);
        if (st_state == closed_fast_state) {    //Chiusura veloce quindi non lo servo e lo sveglio soltanto
            MINUS1(wakeup_client(client, 0), perror("wake up client"); return NULL)
        } else {    //Lo servo e poi lo sveglio
            PTH(err, pthread_mutex_unlock(&(ca->mutex)), perror("unlock"); return NULL)
            MINUS1(serve_client(ca, client), perror("serve client"); return NULL)
            PTH(err, pthread_mutex_lock(&(ca->mutex)), perror("lock"); return NULL)
            //Sveglio il cliente
            MINUS1(wakeup_client(client, 1), perror("wake up client"); return NULL)
        }
    }
    PTH(err, pthread_mutex_unlock(&(ca->mutex)), perror("unlock"); return NULL)
    DEBUG("Cassiere %ld: termino\n", ca->id)
    return 0;
}

static int serve_client(cassiere_t *ca, client_in_queue_t *client) {
    int ms = ca->fixed_service_time + (ca->product_service_time*(client->products));
    DEBUG("Cassiere %ld: Servo un cliente. Impiegherò %dms\n", ca->id, ms)
    MINUS1(msleep(ms), return -1)
    return 0;
}

int set_cassa_state(cassiere_t *cassiere, int open) {
    int err;

    PTH(err, pthread_mutex_lock(&(cassiere->mutex)), return -1)
    if (cassiere->isopen && !open) {
        //Se la cassa è aperta ma voglio chiuderla
        PTH(err, pthread_cond_signal(&(cassiere->noclients)), return -1)
        cassiere->isopen = 0;
    } else if (!(cassiere->isopen) && open) {
        //Se la cassa è chiusa ma voglio aprirla
        PTH(err, pthread_cond_signal(&(cassiere->waiting)), return -1)
        cassiere->isopen = 1;
    }
    PTH(err, pthread_mutex_unlock(&(cassiere->mutex)), return -1)

    return 0;
}

int cassiere_wake_up(cassiere_t *cassiere) {
    int err;

    PTH(err, pthread_mutex_lock(&(cassiere->mutex)), return -1)
    PTH(err, pthread_cond_signal(&(cassiere->noclients)), return -1)
    PTH(err, pthread_cond_signal(&(cassiere->waiting)), return -1)
    PTH(err, pthread_mutex_unlock(&(cassiere->mutex)), return -1)

    return 0;
}