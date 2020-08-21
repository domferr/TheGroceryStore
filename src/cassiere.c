#define DEBUGGING 0
#include "../include/utils.h"
#include "../include/cassiere.h"
#include "../include/store.h"
#include "../include/notifier.h"
#include "../include/client_in_queue.h"
#include "../include/cassa_queue.h"
#include "../include/log.h"
#include <stdlib.h>
#include <stdio.h>

#define MIN_SERVICE_TIME 20 //ms
#define MAX_SERVICE_TIME 80 //ms

/**
 * Serve un cliente. Il tempo impiegato dipende dal tempo fisso del cassiere, dal tempo di elaborazione del singolo
 * prodotto e dal numero di prodotti che il cliente vuole acquistare. Ritorna 0 in caso di successo, -1 altrimenti e
 * imposta errno. Il tempo di servizio del cassiere è definito dalla macro SERVICE_TIME.
 *
 * @param ca struttura dati del cassiere
 * @param client cliente che era in coda e che deve essere servito
 * @param log struttura dati per il logging
 * @return 0 in caso di successo, -1 altrimenti e imposta errno
 */
static int serve_client(cassiere_t *ca, client_in_queue_t *client, cassa_log_t *log);

cassiere_t *alloc_cassiere(size_t id, store_t *store, int isopen, int product_service_time, int interval) {
    int err;
    unsigned int seed = id + time(NULL);

    cassiere_t *cassiere = (cassiere_t*) malloc(sizeof(cassiere_t));
    EQNULL(cassiere, return NULL)

    cassiere->id = id;
    cassiere->isopen = isopen;
    cassiere->store = store;
    cassiere->product_service_time = product_service_time;
    cassiere->fixed_service_time = RANDOM(seed, MIN_SERVICE_TIME, MAX_SERVICE_TIME);  //milliseconds
    cassiere->interval = interval;
    EQNULL(cassiere->cassa_log = alloc_cassa_log(id), return NULL)
    if (cassiere->isopen) {
        MINUS1(log_cassa_open(cassiere->cassa_log), return NULL)
    } else {
        log_cassa_closed(cassiere->cassa_log);
    }
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
    cassa_log_t *cassa_log;
    pthread_t th_notifier;
    store_state st_state;
    notifier_t *notifier = alloc_notifier(ca, ca->isopen);
    EQNULLERR(notifier, return NULL)
    PTHERR(err, pthread_create(&th_notifier, NULL, &notifier_thread_fun, notifier), return NULL)
    EQNULLERR(cassa_log = ca->cassa_log, return NULL)

    MINUS1ERR(get_store_state(ca->store, &st_state), return NULL)
    while(ISOPEN(st_state)) {
        PTHERR(err, pthread_mutex_lock(&(ca->mutex)), return NULL)
        if (ca->isopen) {
            MINUS1ERR(set_notifier_state(notifier, notifier_run), return NULL)
            //esco se il supermercato viene chiuso o se la cassa viene chiusa o se arriva un cliente
            while (ISOPEN(st_state) && ca->isopen && ca->queue->size == 0) {
                PTHERR(err, pthread_cond_wait(&(ca->noclients), &(ca->mutex)), return NULL)
                PTHERR(err, pthread_mutex_unlock(&(ca->mutex)), return NULL)
                MINUS1ERR(get_store_state(ca->store, &st_state), return NULL)
                PTHERR(err, pthread_mutex_lock(&(ca->mutex)), return NULL)
            }
            //C'è un cliente e sia la cassa che il supermercato sono aperti
            if (ISOPEN(st_state) && ca->isopen) {
                client = get_next_client(ca, 1);
                PTHERR(err, pthread_mutex_unlock(&(ca->mutex)), return NULL)
                MINUS1ERR(serve_client(ca, client, cassa_log), return NULL)
                PTHERR(err, pthread_mutex_lock(&(ca->mutex)), return NULL)
                //Sveglio il cliente
                MINUS1ERR(wakeup_client(client, 1), return NULL)
            } else if (!ca->isopen) {   //Se la cassa è stata chiusa, prendo il log
                MINUS1ERR(log_cassa_closed(cassa_log), return NULL)
            }
        } else {
            DEBUG("Cassiere %ld: cassa chiusa, clienti in coda %d PRIMA\n", ca->id, ca->queue->size)
            //Avvisa tutti i clienti in coda che la cassa è chiusa
            while(ca->queue->size > 0) {
                client = get_next_client(ca, 0);
                MINUS1ERR(wakeup_client(client, 0), return NULL)
            }
            DEBUG("Cassiere %ld: cassa chiusa, clienti in coda %d DOPO\n", ca->id, ca->queue->size)
            //Aspetto fino a quando il supermercato chiude o quando il direttore apre la cassa
            while (ISOPEN(st_state) && !ca->isopen) {
                PTHERR(err, pthread_cond_wait(&(ca->waiting), &(ca->mutex)), return NULL)
                PTHERR(err, pthread_mutex_unlock(&(ca->mutex)), return NULL)
                MINUS1ERR(get_store_state(ca->store, &st_state), return NULL)
                PTHERR(err, pthread_mutex_lock(&(ca->mutex)), return NULL)
            }
            if (ca->isopen) { //Se la cassa è stata aperta, prendo il log
                MINUS1ERR(log_cassa_open(cassa_log), return NULL)
            }
        }
        PTHERR(err, pthread_mutex_unlock(&(ca->mutex)), return NULL)
        MINUS1ERR(get_store_state(ca->store, &st_state), return NULL)
    }
    DEBUG("Cassiere %ld: faccio terminare il notifier\n", ca->id)
    MINUS1ERR(set_notifier_state(notifier, notifier_quit), return NULL)
    PTHERR(err, pthread_join(th_notifier, NULL), return NULL)
    MINUS1ERR(destroy_notifier(notifier), return NULL)
    DEBUG("Cassiere %ld: gestisco clienti rimasti in coda\n", ca->id)
    PTHERR(err, pthread_mutex_lock(&(ca->mutex)), return NULL)
    while(ca->queue->size > 0) {
        client = get_next_client(ca, st_state != closed_fast_state);
        if (st_state == closed_fast_state) {    //Chiusura veloce quindi non lo servo e lo sveglio soltanto
            MINUS1ERR(wakeup_client(client, 0), return NULL)
        } else {    //Lo servo e poi lo sveglio
            PTHERR(err, pthread_mutex_unlock(&(ca->mutex)), return NULL)
            MINUS1ERR(serve_client(ca, client, cassa_log), return NULL)
            PTHERR(err, pthread_mutex_lock(&(ca->mutex)), return NULL)
            //Sveglio il cliente
            MINUS1ERR(wakeup_client(client, 1), return NULL)
        }
    }
    MINUS1ERR(log_cassa_store_closed(cassa_log), return NULL)
    PTHERR(err, pthread_mutex_unlock(&(ca->mutex)), return NULL)
    DEBUG("Cassiere %ld: termino\n", ca->id)
    return cassa_log;
}

static int serve_client(cassiere_t *ca, client_in_queue_t *client, cassa_log_t *log) {
    long ms = SERVICE_TIME(ca, client);
    DEBUG("Cassiere %ld: Servo un cliente. Impiegherò %dms\n", ca->id, ms)
    MINUS1(msleep(ms), return -1)
    MINUS1(log_client_served(log, ms, client->products), return -1)
    return 0;
}

int set_cassa_state(cassiere_t *cassiere, int open) {
    int err;

    PTH(err, pthread_mutex_lock(&(cassiere->mutex)), return -1)
    if (cassiere->isopen && !open) {
        //Se la cassa è aperta ma voglio chiuderla
        PTH(err, pthread_cond_signal(&(cassiere->noclients)), return -1)
        cassiere->isopen = 0;
        MINUS1(log_cassa_closed(cassiere->cassa_log), return -1)
    } else if (!(cassiere->isopen) && open) {
        //Se la cassa è chiusa ma voglio aprirla
        PTH(err, pthread_cond_signal(&(cassiere->waiting)), return -1)
        cassiere->isopen = 1;
        MINUS1(log_cassa_open(cassiere->cassa_log), return -1)
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