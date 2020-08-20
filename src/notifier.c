#define DEBUGGING 0
#include <stdlib.h>
#include <stdio.h>
#include "../include/notifier.h"
#include "../include/utils.h"
#include "../include/store.h"

void *notifier_thread_fun(void *args) {
    int err, queue_len;
    notifier_t *no = (notifier_t*) args;
    cassiere_t *ca = no->cassiere;

    //Attendo l'intervallo
    MINUS1ERR(msleep(ca->interval), return NULL)

    PTHERR(err, pthread_mutex_lock(&(no->mutex)), return NULL)
    while(no->state != notifier_quit) {
        //Aspetto di essere attivato dal cassiere
        while(no->state == notifier_pause) {
            PTHERR(err, pthread_cond_wait(&(no->paused), &(no->mutex)), return NULL)
        }
        //Controllo se sono stato attivato perchè il supermercato sta chiudendo
        if (no->state == notifier_run) {
            PTHERR(err, pthread_mutex_unlock(&(no->mutex)), return NULL)

            //Prendo la lunghezza della coda
            PTHERR(err, pthread_mutex_lock(&(ca->mutex)), return NULL)
            queue_len = (ca->queue)->size;
            PTHERR(err, pthread_mutex_unlock(&(ca->mutex)), return NULL)

            //Invio al direttore il numero di clienti in coda
            MINUS1ERR(notify(ca->id, queue_len), return NULL)

            DEBUG("Notificatore %ld: ho inviato una notifica\n", ca->id);
            //Attendo l'intervallo
            MINUS1ERR(msleep(ca->interval), return NULL)

            PTHERR(err, pthread_mutex_lock(&(no->mutex)), return NULL)
        }
    }
    PTHERR(err, pthread_mutex_unlock(&(no->mutex)), return NULL)
    DEBUG("Notificatore %ld: termino\n", ca->id);
    return 0;
}

notifier_t *alloc_notifier(cassiere_t *cassiere, int start_notify) {
    int err;
    notifier_t *notifier = (notifier_t*) malloc(sizeof(notifier_t));
    EQNULL(notifier, return NULL)
    notifier->cassiere = cassiere;
    PTH(err, pthread_mutex_init(&(notifier->mutex), NULL), return NULL)
    PTH(err, pthread_cond_init(&(notifier->paused), NULL), return NULL)
    notifier->state = start_notify? notifier_run:notifier_pause;
    return notifier;
}

int destroy_notifier(notifier_t *notifier) {
    int err;
    PTH(err, pthread_mutex_destroy(&(notifier->mutex)), return -1)
    PTH(err, pthread_cond_destroy(&(notifier->paused)), return -1)
    free(notifier);
    return 0;
}

int set_notifier_state(notifier_t *no, notifier_state new_state) {
    int err;
    PTH(err, pthread_mutex_lock(&(no->mutex)), return -1)
    no->state = new_state;
    PTH(err, pthread_cond_signal(&(no->paused)), return -1)
    PTH(err, pthread_mutex_unlock(&(no->mutex)), return -1)
    return 0;
}