#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <signal.h>
#include "grocerystore.h"

typedef struct {
    sigset_t set;
    grocerystore_t *gs;
} signal_handler_t;

void *thread_handler_fun(void *arg);

#endif //SIGNAL_HANDLER_H
