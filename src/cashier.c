#define _POSIX_C_SOURCE 200809L

#include "cashier.h"
#include "utils.h"
#include "grocerystore.h"
#include "client_in_queue.h"
#include "queue.h"
#include "logger.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

//#define DEBUG_CASHIER
#define MIN_SERVICE_TIME 20 //ms
#define MAX_SERVICE_TIME 80 //ms

static int push_activation_time(cashier_thread_stats *stats, struct timespec *active_start);
static int get_clients_in_queue(cashier_t *ca);
static void *notifier_fun(void *args);
static notifier_t *notifier_create(cashier_t *ca, manager_queue *mq, int interval);
static int destroy_notifier(notifier_t *no);
static int set_notifier_state(notifier_t *notifier, notifier_state state);

void *cashier_fun(void *args) {
    int err;
    struct timespec active_start = {0,0};
    cashier_t *ca = (cashier_t*) args;
    cashier_sync *ca_sync = ca->ca_sync;
    notifier_t *notifier;
    pthread_mutex_t *mutex = &(ca_sync->mutex);
    pthread_cond_t *paused = &(ca_sync->paused), *noclients = &(ca_sync->noclients);
    grocerystore_t *gs = ca->gs;
    gs_state store_state;
    client_in_queue *client;
#ifdef DEBUG_CASHIER
    printf("Cassiere %ld: attivo\n", ca->id);
#endif
    cashier_thread_stats *stats = alloc_cashier_thread_stats(ca->id);
    EQNULL(stats, perror("alloc cashier thread stats"); return NULL)

    notifier = notifier_create(ca, ca->mq, ca->interval);
    EQNULL(notifier, perror("notifier create"); return NULL)
    if (ca_sync->state == active)
        MINUS1(set_notifier_state(notifier, turned_on), perror("set notifier state"); return NULL)

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
                    MINUS1(set_notifier_state(notifier, turned_on), perror("set notifier state"); return NULL)
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
                    MINUS1(set_notifier_state(notifier, turned_off), perror("set notifier state"); return NULL)
                }
                PTH(err, pthread_mutex_unlock(mutex), perror("cashier unlock"); return NULL)
                break;
        }
    }

    //Gestisci chiusura supermercato
    MINUS1(handle_closure(ca, store_state, stats), perror("handle closure"); return NULL)
    MINUS1(set_notifier_state(notifier, stopped), perror("set notifier state"); return NULL)

    //join del thread notificatore
    NOTZERO(pthread_join(notifier->thread, NULL), perror("join notifier thread"); return NULL)
#ifdef DEBUG_CASHIER
    printf("Cassiere %ld: notificatore terminato\n", ca->id);
#endif
    //cleanup
    PTH(err, pthread_mutex_destroy(mutex), perror("cashier mutex destroy"); return NULL)
    PTH(err, pthread_cond_destroy(paused), perror("cashier cond destroy"); return NULL)
    NOTZERO(queue_destroy(ca->queue), perror("cashier queue destroy"); return NULL)
    destroy_notifier(notifier);
    free(ca_sync);
    free(ca);
    return stats;
}

cashier_t *alloc_cashier(size_t id, grocerystore_t *gs, cashier_state starting_state, int product_service_time, int interval, manager_queue *mq) {
    int err;
    unsigned int seed = id;

    cashier_t *ca = (cashier_t*) malloc(sizeof(cashier_t));
    EQNULL(ca, return NULL)

    cashier_sync *ca_sync = (cashier_sync*) malloc(sizeof(cashier_sync));
    EQNULL(ca_sync, return NULL)
    ca_sync->state = starting_state;
    PTH(err, pthread_mutex_init(&(ca_sync->mutex), NULL), return NULL)
    PTH(err, pthread_cond_init(&(ca_sync->paused), NULL), return NULL)
    PTH(err, pthread_cond_init(&(ca_sync->noclients), NULL), return NULL)
    ca->ca_sync = ca_sync;

    ca->id = id;
    ca->gs = gs;
    ca->product_service_time = product_service_time;
    ca->fixed_service_time = RANDOM(seed, MIN_SERVICE_TIME, MAX_SERVICE_TIME);  //milliseconds
    ca->interval = interval;
    ca->mq = mq;

    ca->queue = queue_create();
    EQNULL(ca->queue, return NULL)

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

static int get_clients_in_queue(cashier_t *ca) {
    int err, retval;
    queue_t *queue = ca->queue;
    cashier_sync *ca_sync = ca->ca_sync;
    PTH(err, pthread_mutex_lock(&(ca_sync->mutex)), return -1)
    retval = queue->size;
    PTH(err, pthread_mutex_unlock(&(ca_sync->mutex)), return -1)
    return retval;
}

/************************* Notificatore *************************/
/**
 * Funzione svolta dal thread notificatore
 *
 * @param args argomenti passati alla funzione
 */
static void *notifier_fun(void *args) {
    int err, run = 1, clients_in_queue;
    notifier_t *notifier = (notifier_t*) args;
    cashier_t *ca = notifier->ca;

    while(run) {
        MINUS1(msleep(ca->interval), perror("notifier msleep"); return NULL);
        //Aspetta di essere attivato
        PTH(err, pthread_mutex_lock(&(notifier->mutex)), perror("notifier mutex lock"); return NULL)
        while (notifier->state == turned_off) {
            PTH(err, pthread_cond_wait(&(notifier->paused), &(notifier->mutex)), perror("notifier cond wait"); return NULL)
        }
        run = notifier->state != stopped;
        PTH(err, pthread_mutex_unlock(&(notifier->mutex)), perror("notifier mutex unlock"); return NULL)
        //Se sono uscito è perchè la cassa è stata chiusa o sono stato fermato
        if (run) {  //Se non sono stato fermato
            //Prendi il numero di clienti in coda
            clients_in_queue = get_clients_in_queue(ca);
            MINUS1(clients_in_queue, perror("notifier get clients in queue"); return NULL)
            //Notifica il direttore
            MINUS1(notify(ca, clients_in_queue), perror("notify"); return NULL)
        }
    }

    return 0;
}

int notify(cashier_t *ca, int clients_in_queue) {
    int err;
    manager_queue *mq = ca->mq;
    queue_t *queue = mq->queue;

    notification_data *data = (notification_data*) malloc(sizeof(notification_data));
    EQNULL(data, return -1);
    data->id = ca->id;
    data->clients_in_queue = clients_in_queue;

    printf("Dico al direttore che ci sono %d clienti\n", clients_in_queue);

    PTH(err, pthread_mutex_lock(&(mq->mutex)), return -1)
    MINUS1(push(queue, data), return -1)
    PTH(err, pthread_cond_signal(&(mq->waiting)), return -1)
    PTH(err, pthread_mutex_unlock(&(mq->mutex)), return -1)
    return 0;
}

static notifier_t *notifier_create(cashier_t *ca, manager_queue *mq, int interval) {
    int err;
    notifier_t *notifier = (notifier_t*) malloc(sizeof(notifier_t));
    EQNULL(notifier, return NULL);
    notifier->ca = ca;
    notifier->state = turned_off;
    PTH(err, pthread_mutex_init(&(notifier->mutex), NULL), return NULL)
    PTH(err, pthread_cond_init(&(notifier->paused), NULL), return NULL)

    NOTZERO(pthread_create(&(notifier->thread), NULL, &notifier_fun, notifier), return NULL);

    return notifier;
}

static int destroy_notifier(notifier_t *no) {
    int err;
    PTH(err, pthread_mutex_destroy(&(no->mutex)), return -1)
    PTH(err, pthread_cond_destroy(&(no->paused)), return -1)
    free(no);
    return 0;
}

static int set_notifier_state(notifier_t *notifier, notifier_state state) {
    int err;
    PTH(err, pthread_mutex_lock(&(notifier->mutex)), perror("notifier mutex lock"); return -1)
    if (notifier->state == turned_off && state != turned_off) {
        PTH(err, pthread_cond_signal(&(notifier->paused)), return -1)
    }
    notifier->state = state;
    PTH(err, pthread_mutex_unlock(&(notifier->mutex)), perror("notifier mutex unlock"); return -1)
    return 0;
}