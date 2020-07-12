#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H
#define _POSIX_C_SOURCE 200809L
#include "grocerystore.h"
#include "cashier.h"
#include <signal.h>

typedef struct {
    sigset_t set;
    grocerystore_t *gs;
    cashier_t **cashiers;
    size_t no_of_cashiers;
} signal_handler_t;

void *thread_handler_fun(void *arg);

#endif //SIGNAL_HANDLER_H
