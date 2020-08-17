#ifndef CLIENT_IN_QUEUE_H
#define CLIENT_IN_QUEUE_H

#include "storetypes.h"

//TODO fare questa documentazione
client_in_queue_t *alloc_client_in_queue(void);

int destroy_client_in_queue(client_in_queue_t *clq);

int wakeup_client(client_in_queue_t *clq, int served);

#endif //CLIENT_IN_QUEUE_H
