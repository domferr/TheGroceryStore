#ifndef NOTIFIER_H
#define NOTIFIER_H

#include "storetypes.h"

typedef enum {
    notifier_run,
    notifier_pause,
    notifier_quit
} notifier_state;

typedef struct {
    cassiere_t *cassiere;
    pthread_mutex_t mutex;
    pthread_cond_t paused;
    notifier_state state;
} notifier_t;

notifier_t *alloc_notifier(cassiere_t *cassiere, int start_notify);

int destroy_notifier(notifier_t *notifier);

void *notifier_thread_fun(void *args);

int set_notifier_state(notifier_t *no, notifier_state new_state);

#endif //NOTIFIER_H
