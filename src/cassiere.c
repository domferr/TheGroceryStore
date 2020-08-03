#include "../include/utils.h"
#include "../include/cassiere.h"
#include "../include/store.h"
#include "../include/notifier.h"
#include <stdlib.h>
#include <stdio.h>

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

    return cassiere;
}

int cassiere_destroy(cassiere_t *cassiere) {
    int err;
    MINUS1(queue_destroy(cassiere->queue), return -1)
    PTH(err, pthread_mutex_destroy(&(cassiere->mutex)), return -1)
    free(cassiere);
    return 0;
}

void *cassiere_thread_fun(void *args) {
    cassiere_t *ca = (cassiere_t*) args;
    store_t *store = ca->store;
    int err;
    pthread_t th_notifier;
    store_state st_state;
    notifier_t *notifier = alloc_notifier(ca);
    EQNULL(notifier, perror("alloc notifier"); exit(EXIT_FAILURE))
    PTH(err, pthread_create(&th_notifier, NULL, &notifier_thread_fun, notifier), perror("create notifier thread"); return NULL)

    NOTZERO(get_store_state(store, &st_state), perror("get store state"); return NULL)
    while(ISOPEN(st_state)) {
        break;
    }
    PTH(err, pthread_join(th_notifier, NULL), perror("join notifier"); return NULL)
    MINUS1(destroy_notifier(notifier), perror("destroy notifier"); return NULL)
    return 0;
}