#include "grocerystore.h"
#include "client.h"
#include "cashier.h"
#include "signal_handler.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define DEBUG_GS

grocerystore_t *grocerystore_create(size_t c) {
    grocerystore_t *gs = (grocerystore_t*) malloc(sizeof(grocerystore_t));
    if (gs == NULL) return NULL;

    gs->state = open;
    gs->clients_inside = 0;
    gs->can_enter = c;
    if (pthread_mutex_init(&(gs->mutex), NULL) != 0) {
        free(gs);
        return NULL;
    }
    if (pthread_cond_init(&(gs->entrance), NULL) != 0) {
        pthread_mutex_destroy(&(gs->mutex));
        free(gs);
        return NULL;
    }
    if (pthread_cond_init(&(gs->exit), NULL) != 0) {
        pthread_cond_destroy(&(gs->entrance));
        pthread_mutex_destroy(&(gs->mutex));
        free(gs);
        return NULL;
    }
    return gs;
}

gs_state doBusiness(grocerystore_t *gs, int c, int e) {
    gs_state state_copy = open;
    while (ISOPEN(state_copy)) {
        pthread_mutex_lock(&(gs->mutex));
        //Rimango in attesa fino a quando il supermercato è aperto ed il numero di clienti è maggiore di C-E
        while (ISOPEN(gs->state) && gs->clients_inside > c-e) {
            pthread_cond_wait(&(gs->exit), &(gs->mutex));
        }
#ifdef DEBUG_GS
        printf("Clienti nel supermercato: %d. Faccio entrare %d clienti\n", gs->clients_inside, e);
#endif
        if (ISOPEN(gs->state)) {
            gs->can_enter = e;
            pthread_cond_broadcast(&(gs->entrance));    //Sveglio tutti i thread in attesa sull'entrata
        }
        state_copy = gs->state;
        pthread_mutex_unlock(&(gs->mutex));
    }
    return state_copy;
}

gs_state enter_store(grocerystore_t *gs, size_t id) {
    gs_state state;
    pthread_mutex_lock(&(gs->mutex));
    while(ISOPEN(gs->state) && gs->can_enter == 0) {
        pthread_cond_wait(&(gs->entrance), &(gs->mutex));
    }
    //Entro se è aperto altrimenti sono stato risvegliato perchè devo terminare
    if (ISOPEN(gs->state)) {
        (gs->can_enter)--;
        (gs->clients_inside)++;
    }
    state = gs->state;
    pthread_mutex_unlock(&(gs->mutex));
    return state;
}

void exit_store(grocerystore_t *gs) {
    pthread_mutex_lock(&(gs->mutex));
    (gs->clients_inside)--;
    pthread_cond_signal(&(gs->exit));
    pthread_mutex_unlock(&(gs->mutex));
}