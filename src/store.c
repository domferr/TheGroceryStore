#define DEBUGGING 0
#include "../include/store.h"
#include "../libs/utils/include/utils.h"
#include <stdlib.h>
#include <stdio.h>

store_t *store_create(size_t c, size_t e) {
    int err;
    store_t *store = (store_t*) malloc(sizeof(store_t));
    EQNULL(store, return NULL)

    //All'inizio ne vengono fatti entrare c, ovvero tutti i clienti entrano contemporaneamente
    store->clients_outside = c;
    store->group_size = e;
    store->can_enter = c;
    store->state = open_state;
    store->clients_counter = 0;
    PTH(err, pthread_mutex_init(&(store->store_mtx), NULL), return NULL)
    PTH(err, pthread_cond_init(&(store->entrance), NULL), return NULL)

    return store;
}

int store_destroy(store_t *store) {
    int err;
    PTH(err, pthread_mutex_destroy(&(store->store_mtx)), return -1)
    PTH(err, pthread_cond_destroy(&(store->entrance)), return -1)
    free(store);
    return 0;
}

int get_store_state(store_t *store, store_state *st_state) {
    int err;
    PTH(err, pthread_mutex_lock(&(store->store_mtx)), return -1)
    *st_state = store->state;
    PTH(err, pthread_mutex_unlock(&(store->store_mtx)), return -1)
    return 0;
}

int close_store(store_t *store, store_state closing_state) {
    int err;
    PTH(err, pthread_mutex_lock(&(store->store_mtx)), return -1)
    if (store->state == open_state) {
        store->state = closing_state;
        store->can_enter = 0;
        PTH(err, pthread_cond_broadcast(&(store->entrance)), return -1)
    }
    PTH(err, pthread_mutex_unlock(&(store->store_mtx)), return -1)
    return 0;
}

int enter_store(store_t *store) {
    int err, id = 0;
    PTH(err, pthread_mutex_lock(&(store->store_mtx)), return -1)
    while(ISOPEN(store->state) && store->can_enter == 0) {
        PTH(err, pthread_cond_wait(&(store->entrance), &(store->store_mtx)), return -1)
    }
    //Entro se è aperto altrimenti sono stato risvegliato perchè devo terminare
    if (ISOPEN(store->state)) {
        store->can_enter--;
        store->clients_outside--;
        store->clients_counter++;
        id = store->clients_counter;
        DEBUG("Entro, ne possono entrare altri %d\n", store->can_enter);
    }
    PTH(err, pthread_mutex_unlock(&(store->store_mtx)), return -1)
    return id;
}

int leave_store(store_t *store) {
    int err;
    PTH(err, pthread_mutex_lock(&(store->store_mtx)), return -1);
    store->clients_outside++;
    if (store->can_enter == 0 && store->clients_outside == store->group_size) {
        store->can_enter = store->group_size;
        PTH(err, pthread_cond_broadcast(&(store->entrance)), return -1)
        DEBUG("Sono usciti %d clienti dal supermercato. Faccio entrare altri %d clienti\n", store->clients_outside, store->group_size);
    }
    PTH(err, pthread_mutex_unlock(&(store->store_mtx)), return -1)
    return 0;
}