#include "grocerystore.h"
#include "client.h"
#include "cashier.h"
#include "signal_handler.h"
#include "utils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

//#define DEBUG_GS

grocerystore_t *grocerystore_create(size_t c) {
    int err;
    grocerystore_t *gs = (grocerystore_t*) malloc(sizeof(grocerystore_t));
    EQNULL(gs, return NULL)

    gs->state = open;
    gs->clients_inside = 0;
    gs->can_enter = c;
    gs->total_clients = 0;
    PTH(err, pthread_mutex_init(&(gs->mutex), NULL), return NULL)
    PTH(err, pthread_cond_init(&(gs->entrance), NULL), return NULL)
    PTH(err, pthread_cond_init(&(gs->exit), NULL), return NULL)

    return gs;
}

int grocerystore_destroy(grocerystore_t *gs) {
    int err;
    PTH(err, pthread_mutex_destroy(&(gs->mutex)), return -1);
    PTH(err, pthread_cond_destroy(&(gs->entrance)), return -1);
    PTH(err, pthread_cond_destroy(&(gs->exit)), return -1);
    free(gs);
    return 0;
}

int manage_entrance(grocerystore_t *gs, gs_state *state, int c, int e) {
    int err, run = 1;
    while (run) {
        PTH(err, pthread_mutex_lock(&(gs->mutex)), return -1)
        //Rimango in attesa fino a quando il supermercato è aperto ed il numero di clienti è maggiore di C-E
        while (ISOPEN(gs->state) && gs->clients_inside > c-e) {
            PTH(err, pthread_cond_wait(&(gs->exit), &(gs->mutex)), return -1)
        }
#ifdef DEBUG_GS
        printf("Clienti nel supermercato: %d. Faccio entrare %d clienti\n", gs->clients_inside, e);
#endif
        if (ISOPEN(gs->state)) {
            gs->can_enter = e;
            //Sveglio tutti i thread in attesa sull'entrata. Solo E di loro entreranno
            PTH(err, pthread_cond_broadcast(&(gs->entrance)), return -1)
        } else {
            run = 0;
        }
        *state = gs->state;
        PTH(err, pthread_mutex_unlock(&(gs->mutex)), return -1)
    }
    return 0;
}

int enter_store(grocerystore_t *gs, gs_state *state) {
    int err, entered = 0;
    PTH(err, pthread_mutex_lock(&(gs->mutex)), return -1)
    while(ISOPEN(gs->state) && gs->can_enter == 0) {
        PTH(err, pthread_cond_wait(&(gs->entrance), &(gs->mutex)), return -1)
    }
    *state = gs->state;
    //Entro se è aperto altrimenti sono stato risvegliato perchè devo terminare
    if (ISOPEN(gs->state)) {
        (gs->can_enter)--;
        (gs->clients_inside)++;
        entered = 1;
    }
    PTH(err, pthread_mutex_unlock(&(gs->mutex)), return -1)
    return entered;
}

int exit_store(grocerystore_t *gs) {
    int err;
    PTH(err, pthread_mutex_lock(&(gs->mutex)), return -1);
    (gs->clients_inside)--;
    PTH(err, pthread_cond_signal(&(gs->exit)), return -1);
    PTH(err, pthread_mutex_unlock(&(gs->mutex)), return -1);
    return 0;
}

int get_store_state(grocerystore_t *gs, gs_state *state) {
    int err;
    PTH(err, pthread_mutex_lock(&(gs->mutex)), return -1)
    *state = gs->state;
    PTH(err, pthread_mutex_unlock(&(gs->mutex)), return -1)
    return 0;
}