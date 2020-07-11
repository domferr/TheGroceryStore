#include "grocerystore.h"
#include "client.h"
#include "cashier.h"
#include "signal_handler.h"
#include "utils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define DEBUG_GS

grocerystore_t *grocerystore_create(size_t c) {
    int err;
    grocerystore_t *gs = (grocerystore_t*) malloc(sizeof(grocerystore_t));
    EQNULL(gs, return NULL)

    gs->state = open;
    gs->clients_inside = 0;
    gs->can_enter = c;
    PTH(err, pthread_mutex_init(&(gs->mutex), NULL), free(gs); return NULL)
    PTH(err, pthread_cond_init(&(gs->entrance), NULL), pthread_mutex_destroy(&(gs->mutex)); free(gs); return NULL)
    if (pthread_cond_init(&(gs->exit), NULL) != 0) {
        pthread_cond_destroy(&(gs->entrance));
        pthread_mutex_destroy(&(gs->mutex));
        free(gs);
        return NULL;
    }
    return gs;
}

gs_state doBusiness(grocerystore_t *gs, int c, int e) {
    int err;
    gs_state state_copy = open;
    while (ISOPEN(state_copy)) {
        PTH(err, pthread_mutex_lock(&(gs->mutex)), return gserror)
        //Rimango in attesa fino a quando il supermercato è aperto ed il numero di clienti è maggiore di C-E
        while (ISOPEN(gs->state) && gs->clients_inside > c-e) {
            PTH(err, pthread_cond_wait(&(gs->exit), &(gs->mutex)), return gserror)
        }
#ifdef DEBUG_GS
        printf("Clienti nel supermercato: %d. Faccio entrare %d clienti\n", gs->clients_inside, e);
#endif
        if (ISOPEN(gs->state)) {
            gs->can_enter = e;
            //Sveglio tutti i thread in attesa sull'entrata
            PTH(err, pthread_cond_broadcast(&(gs->entrance)), return gserror)
        }
        state_copy = gs->state;
        PTH(err, pthread_mutex_unlock(&(gs->mutex)), return gserror)
    }
    return state_copy;
}

gs_state enter_store(grocerystore_t *gs, size_t id) {
    int err;
    gs_state state;
    PTH(err, pthread_mutex_lock(&(gs->mutex)), return gserror)
    while(ISOPEN(gs->state) && gs->can_enter == 0) {
        PTH(err, pthread_cond_wait(&(gs->entrance), &(gs->mutex)), return gserror)
    }
    //Entro se è aperto altrimenti sono stato risvegliato perchè devo terminare
    if (ISOPEN(gs->state)) {
        (gs->can_enter)--;
        (gs->clients_inside)++;
    }
    state = gs->state;
    PTH(err, pthread_mutex_unlock(&(gs->mutex)), return gserror)
    return state;
}

int exit_store(grocerystore_t *gs) {
    int err;
    PTH(err, pthread_mutex_lock(&(gs->mutex)), return -1);
    (gs->clients_inside)--;
    PTH(err, pthread_cond_signal(&(gs->exit)), return -1);
    PTH(err, pthread_mutex_unlock(&(gs->mutex)), return -1);
    return 0;
}