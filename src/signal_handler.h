#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#define _POSIX_C_SOURCE 200809L
#include "grocerystore.h"
#include "cashier.h"
#include "manager.h"
#include <signal.h>

typedef struct {
    sigset_t set;
    grocerystore_t *gs;
    cashier_sync **cashiers;
    size_t no_of_cashiers;
    manager_queue *mqueue;
} signal_handler_t;

void *thread_handler_fun(void *arg);
int setup_signal_handling(pthread_t *handler, manager_queue *mqueue, grocerystore_t *gs, cashier_t **cashiers_args, size_t no_of_cashiers);

#endif //SIGNAL_HANDLER_H
