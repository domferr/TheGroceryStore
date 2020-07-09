#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <signal.h>

typedef struct {
    sigset_t set;
} signal_handler_t;

void *signal_handler_fun(void *arg);

#endif //SIGNAL_HANDLER_H
