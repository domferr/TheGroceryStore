#include "../include/utils.h"
#include "../include/cassiere.h"
#include <stdlib.h>

#define MIN_SERVICE_TIME 20 //ms
#define MAX_SERVICE_TIME 80 //ms

cassiere_t *alloc_cassiere(size_t id, store_t *store, int fd, pthread_mutex_t *fd_mtx, int product_service_time, int interval) {
    unsigned int seed = id;

    cassiere_t *cassiere = (cassiere_t*) malloc(sizeof(cassiere_t));
    EQNULL(cassiere, return NULL)

    cassiere->id = id;
    cassiere->store = store;
    cassiere->fd = fd;
    cassiere->fd_mtx = fd_mtx;
    cassiere->product_service_time = product_service_time;
    cassiere->fixed_service_time = RANDOM(seed, MIN_SERVICE_TIME, MAX_SERVICE_TIME);  //milliseconds
    cassiere->interval = interval;
    EQNULL(cassiere->queue = queue_create(), return NULL)

    return cassiere;
}

int cassiere_destroy(cassiere_t *cassiere) {
    MINUS1(queue_destroy(cassiere->queue), return -1)
    free(cassiere);
    return 0;
}

void *cassiere_thread_fun(void *args) {
    return 0;
}