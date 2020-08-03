#ifndef NOTIFIER_H
#define NOTIFIER_H

#include "storetypes.h"

typedef struct {
    cassiere_t *cassiere;
    pthread_mutex_t mutex;
    pthread_cond_t paused;
    int on_pause;
} notifier_t;

notifier_t *alloc_notifier(cassiere_t *cassiere);

int destroy_notifier(notifier_t *notifier);

void *notifier_thread_fun(void *args);

#endif //NOTIFIER_H
