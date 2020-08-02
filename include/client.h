#ifndef CLIENT_H
#define CLIENT_H

#include "../src/storetypes.h"

void *client_thread_fun(void *args);

client_t *alloc_client(size_t id, store_t *store, int t, int p, int s, int shared_pipe[2], size_t no_of_cashiers);

//int enter_random_queue(client_t *cl, client_in_queue *cl_in_q, grocerystore_t *gs, gs_state *store_state);

#endif //CLIENT_H
