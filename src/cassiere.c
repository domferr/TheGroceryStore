#include "../include/utils.h"
#include "../include/cassiere.h"
#include "../include/store.h"
#include "../include/notifier.h"
#include <stdlib.h>
#include <stdio.h>

#define DEBUG_CASSIERE
#define MIN_SERVICE_TIME 20 //ms
#define MAX_SERVICE_TIME 80 //ms

cassiere_t *alloc_cassiere(size_t id, store_t *store, int isopen, int fd, pthread_mutex_t *fd_mtx, int product_service_time, int interval) {
    int err;
    unsigned int seed = id;

    cassiere_t *cassiere = (cassiere_t*) malloc(sizeof(cassiere_t));
    EQNULL(cassiere, return NULL)

    cassiere->id = id;
    cassiere->state = isopen ? cassa_open_state:cassa_closed_state;
    cassiere->store = store;
    cassiere->fd = fd;
    cassiere->fd_mtx = fd_mtx;
    cassiere->product_service_time = product_service_time;
    cassiere->fixed_service_time = RANDOM(seed, MIN_SERVICE_TIME, MAX_SERVICE_TIME);  //milliseconds
    cassiere->interval = interval;
    EQNULL(cassiere->queue = queue_create(), return NULL)
    PTH(err, pthread_mutex_init(&(cassiere->mutex), NULL), return NULL)
    PTH(err, pthread_cond_init(&(cassiere->noclients), NULL), return NULL)
    PTH(err, pthread_cond_init(&(cassiere->waiting), NULL), return NULL)
    return cassiere;
}

int cassiere_destroy(cassiere_t *cassiere) {
    int err;
    MINUS1(queue_destroy(cassiere->queue), return -1)
    PTH(err, pthread_mutex_destroy(&(cassiere->mutex)), return -1)
    PTH(err, pthread_cond_destroy(&(cassiere->noclients)), return -1)
    PTH(err, pthread_cond_destroy(&(cassiere->waiting)), return -1)
    free(cassiere);
    return 0;
}

void *cassiere_thread_fun(void *args) {
    cassiere_t *ca = (cassiere_t*) args;
    store_t *store = ca->store;
    int err, serve;
    void *client;
    pthread_t th_notifier;
    store_state st_state;
    notifier_t *notifier = alloc_notifier(ca, ca->state == cassa_open_state);
    EQNULL(notifier, perror("alloc notifier"); exit(EXIT_FAILURE))
    PTH(err, pthread_create(&th_notifier, NULL, &notifier_thread_fun, notifier), perror("create notifier thread"); return NULL)

    NOTZERO(get_store_state(store, &st_state), perror("get store state"); return NULL)
    while(ISOPEN(st_state)) {
        PTH(err, pthread_mutex_lock(&(ca->mutex)), perror("lock"); return NULL)
        switch (ca->state) {
            case cassa_closed_state:
                //avvisa i clienti in coda che la cassa è chiusa
                while((client = pop(ca->queue)) != NULL) {
                    //TODO avvisa il cliente in coda che la cassa è chiusa
                    //wakeup_client(client, 0);
                }
                //Aspetto fino a quando non vengo aperto perchè il supermercato chiude o perchè il direttore apre la cassa
                while (ISOPEN(st_state) && ca->state == cassa_closed_state) {
                    PTH(err, pthread_cond_wait(&(ca->waiting), &(ca->mutex)), perror("cond wait"); return NULL)
                    PTH(err, pthread_mutex_unlock(&(ca->mutex)), perror("unlock"); return NULL)
                    NOTZERO(get_store_state(store, &st_state), perror("get store state"); return NULL)
                    PTH(err, pthread_mutex_lock(&(ca->mutex)), perror("lock"); return NULL)
                }
                break;
            case cassa_open_state:
                PTH(err, pthread_mutex_unlock(&(ca->mutex)), perror("unlock"); return NULL)
                MINUS1(set_notifier_state(notifier, notifier_run), perror("set notifier state"); return NULL)

                PTH(err, pthread_mutex_lock(&(ca->mutex)), perror("lock"); return NULL)
                //esco se lo store viene chiuso o se la cassa viene chiusa o se arriva un cliente
                //TODO segnalare su noclients quando chiudo la cassa, quando chiudo il supermercato, quando arriva un cliente
                while (ISOPEN(st_state) && ca->state == cassa_open_state && ca->queue->size == 0) {
                    PTH(err, pthread_cond_wait(&(ca->noclients), &(ca->mutex)), perror("cond wait"); return NULL)
                    PTH(err, pthread_mutex_unlock(&(ca->mutex)), perror("unlock"); return NULL)
                    NOTZERO(get_store_state(store, &st_state), perror("get store state"); return NULL)
                    PTH(err, pthread_mutex_lock(&(ca->mutex)), perror("lock"); return NULL)
                }
                //qua ho la lock del cassiere
                //Caso in cui è arrivato un cliente e sia la cassa che il supermercato sono aperti
                if (ISOPEN(st_state) && ca->state == cassa_open_state) {
                    //client = pop(ca->queue);
                    //serve_client(client);
                }
                break;
        }
        PTH(err, pthread_mutex_unlock(&(ca->mutex)), perror("unlock"); return NULL)
        NOTZERO(get_store_state(store, &st_state), perror("get store state"); return NULL)
    }

#ifdef DEBUG_CASSIERE
    printf("Cassiere %ld: gestisco clienti rimasti in coda\n", ca->id);
#endif
    while((client = pop(ca->queue)) != NULL) {
        if (st_state != closed_fast_state) { //TODO gestire tutti i clienti ancora in coda e poi terminare
            //serve_client(client);
        } else {
            //wakeup_client(client, 0);
        }
    }
#ifdef DEBUG_CASSIERE
    printf("Cassiere %ld: faccio terminare il notifier\n", ca->id);
#endif
    MINUS1(set_notifier_state(notifier, notifier_quit), perror("set notifier state"); return NULL)
    PTH(err, pthread_join(th_notifier, NULL), perror("join notifier"); return NULL)
    MINUS1(destroy_notifier(notifier), perror("destroy notifier"); return NULL)
#ifdef DEBUG_CASSIERE
    printf("Cassiere %ld: termino\n", ca->id);
#endif
    return 0;
}

int enter_cassa_queue(cassiere_t *cassiere, void *cliente) {
    int err;

    PTH(err, pthread_mutex_lock(&(cassiere->mutex)), return -1)
    if (cassiere->state == cassa_open_state) {
        MINUS1(push(cassiere->queue, cliente), return -1)
        PTH(err, pthread_cond_signal(&(cassiere->noclients)), return -1)
    }
    PTH(err, pthread_mutex_unlock(&(cassiere->mutex)), return -1)

    return 0;
}

int set_cassa_state(cassiere_t *cassiere, int open) {
    int err;

    PTH(err, pthread_mutex_lock(&(cassiere->mutex)), return -1)
    if (cassiere->state == cassa_open_state && !open) {
        //Se la cassa è aperta ma voglio chiuderla
        PTH(err, pthread_cond_signal(&(cassiere->noclients)), return -1)
        cassiere->state = cassa_closed_state;
    } else if (cassiere->state == cassa_closed_state && open) {
        //Se la cassa è chiusa ma voglio aprirla
        PTH(err, pthread_cond_signal(&(cassiere->waiting)), return -1)
        cassiere->state = cassa_open_state;
    }
    PTH(err, pthread_mutex_unlock(&(cassiere->mutex)), return -1)

    return 0;
}

int cassiere_quit(cassiere_t *cassiere) {
    int err;

    PTH(err, pthread_mutex_lock(&(cassiere->mutex)), return -1)
    PTH(err, pthread_cond_signal(&(cassiere->noclients)), return -1)
    PTH(err, pthread_cond_signal(&(cassiere->waiting)), return -1)
    PTH(err, pthread_mutex_unlock(&(cassiere->mutex)), return -1)

    return 0;
}