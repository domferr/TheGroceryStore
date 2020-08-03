#include <stdlib.h>
#include <stdio.h>
#include "../include/notifier.h"
#include "../include/utils.h"
#include "../include/af_unix_conn.h"
#include "../include/store.h"

void *notifier_thread_fun(void *args) {
    int err, queue_len;
    notifier_t *no = (notifier_t*) args;
    cassiere_t *ca = no->cassiere;
    store_t *store = ca->store;
    store_state st_state;
    //Attendo l'intervallo
    msleep(ca->interval);
    NOTZERO(get_store_state(store, &st_state), perror("get store state"); return NULL)
    while(ISOPEN(st_state)) {
        //Prendo la lunghezza della coda
        PTH(err, pthread_mutex_lock(&(ca->mutex)), perror("lock"); return NULL)
        queue_len = (ca->queue)->size;
        PTH(err, pthread_mutex_unlock(&(ca->mutex)), perror("unlock"); return NULL)

        //Invio al direttore il numero di clienti in coda
        PTH(err, pthread_mutex_lock(ca->fd_mtx), perror("lock"); return NULL)
        notify(ca->fd, ca->id, queue_len);
        PTH(err, pthread_mutex_unlock(ca->fd_mtx), perror("unlock"); return NULL)

        //Attendo l'intervallo
        msleep(ca->interval);
        NOTZERO(get_store_state(store, &st_state), perror("get store state"); return NULL)
    }

    return 0;
}

notifier_t *alloc_notifier(cassiere_t *cassiere) {
    int err;
    notifier_t *notifier = (notifier_t*) malloc(sizeof(notifier_t));
    EQNULL(notifier, return NULL)
    notifier->cassiere = cassiere;
    PTH(err, pthread_mutex_init(&(notifier->mutex), NULL), return NULL);
    PTH(err, pthread_cond_init(&(notifier->paused), NULL), return NULL);
    notifier->on_pause = 0;
    return notifier;
}

int destroy_notifier(notifier_t *notifier) {
    int err;
    PTH(err, pthread_mutex_destroy(&(notifier->mutex)), return -1)
    PTH(err, pthread_cond_destroy(&(notifier->paused)), return -1)
    free(notifier);
    return 0;
}
