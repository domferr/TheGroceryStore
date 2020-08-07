#include "../include/store.h"
#include "../include/utils.h"
#include <stdlib.h>
#include <stdio.h>

//#define DEBUGSTORE
//TODO valutare se tenerlo globale o se togliere proprio la struct dello store
pthread_mutex_t store_mtx = PTHREAD_MUTEX_INITIALIZER;  //mutex per operazioni sullo store in mutua esclusione
//pthread_cond_t entrance = PTHREAD_COND_INITIALIZER;
static store_state state = open_state;                  //stato dello store. Inizialmente chiuso
/*static int clients_outside;
static int group_size;
static int can_enter;

int open_store(size_t c, size_t e) {
    int err;
    PTH(err, pthread_mutex_lock(&store_mtx), return -1)
    if (!(ISOPEN(state))) { //apro solo se non è già aperto
        clients_outside = c;
        group_size = e;
        can_enter = c;      //All'inizio ne vengono fatti entrare c, ovvero tutti i clienti entrano contemporaneamente
        state = open_state;
    }
    PTH(err, pthread_mutex_unlock(&store_mtx), return -1)
    return 0;
}*/

store_t *store_create(size_t c, size_t e) {
    int err;
    store_t *store = (store_t*) malloc(sizeof(store_t));
    EQNULL(store, return NULL)

    //All'inizio ne vengono fatti entrare c, ovvero tutti i clienti entrano contemporaneamente
    store->clients_outside = c;
    store->group_size = e;
    store->can_enter = c;

    PTH(err, pthread_cond_init(&(store->entrance), NULL), return NULL)

    return store;
}

int store_destroy(store_t *store) {
    int err;
    PTH(err, pthread_mutex_destroy(&store_mtx), return -1)
    PTH(err, pthread_cond_destroy(&(store->entrance)), return -1)
    free(store);
    return 0;
}

int get_store_state(store_state *st_state) {
    int err;
    PTH(err, pthread_mutex_lock(&store_mtx), return -1)
    *st_state = state;
    PTH(err, pthread_mutex_unlock(&store_mtx), return -1)
    return 0;
}

int close_store(store_t *store, store_state closing_state) {
    int err;
    PTH(err, pthread_mutex_lock(&store_mtx), return -1)
    if (state == open_state) {
        state = closing_state;
        store->can_enter = 0;
    }
    PTH(err, pthread_cond_broadcast(&(store->entrance)), return -1)
    PTH(err, pthread_mutex_unlock(&store_mtx), return -1)
    return 0;
}

int enter_store(store_t *store) {
    int err;
    static int client_id = 0;
    PTH(err, pthread_mutex_lock(&store_mtx), return -1)
    while(ISOPEN(state) && store->can_enter == 0) {
        PTH(err, pthread_cond_wait(&(store->entrance), &store_mtx), return -1)
    }
    //Entro se è aperto altrimenti sono stato risvegliato perchè devo terminare
    if (ISOPEN(state)) {
        (store->can_enter)--;
        (store->clients_outside)--;
        client_id++;// = store->total_clients;
#ifdef DEBUGSTORE
        printf("Entro, ne possono entrare altri %d\n", store->can_enter);
#endif
    } else {
        client_id = 0;
    }
    PTH(err, pthread_mutex_unlock(&store_mtx), return -1)
    return client_id;
}

int leave_store(store_t *store) {
    int err;
    PTH(err, pthread_mutex_lock(&store_mtx), return -1);
    (store->clients_outside)++;
    if (store->can_enter == 0 && store->clients_outside == store->group_size) {
        store->can_enter = store->group_size;
        PTH(err, pthread_cond_broadcast(&(store->entrance)), return -1)
#ifdef DEBUGSTORE
        printf("Ci sono %d clienti nel supermercato. Faccio entrare altri %d clienti\n", store->clients_inside, store->group_size);
#endif
    }
    PTH(err, pthread_mutex_unlock(&store_mtx), return -1)
    return 0;
}