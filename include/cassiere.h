#ifndef CASSIERE_H
#define CASSIERE_H

#include "storetypes.h"

void *cassiere_thread_fun(void *args);

cassiere_t *alloc_cassiere(size_t id, store_t *store, int isopen, int fd, pthread_mutex_t *fd_mtx, int product_service_time, int interval);

int cassiere_destroy(cassiere_t *cassiere);

int set_cassa_state(cassiere_t *cassiere, int open);

int cassiere_quit(cassiere_t *cassiere);

#endif //CASSIERE_H
