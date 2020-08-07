#ifndef CLIENT_IN_QUEUE_H
#define CLIENT_IN_QUEUE_H

#include "storetypes.h"

//TODO fare questa documentazione
client_in_queue *alloc_client_in_queue(size_t id, pthread_mutex_t *mutex);

int destroy_client_in_queue(client_in_queue *cl_in_q);

int wakeup_client(client_in_queue *clq, int served);

#endif //CLIENT_IN_QUEUE_H
