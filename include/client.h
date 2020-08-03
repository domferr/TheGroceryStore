#ifndef CLIENT_H
#define CLIENT_H

#include "../src/storetypes.h"

void *client_thread_fun(void *args);

client_t *alloc_client(size_t id, store_t *store, int t, int p, int s, int fd, int k);

int client_destroy(client_t *client);

int set_exit_permission(client_t *client, int can_exit);

int wait_permission(client_t *cl);

int enter_best_queue(struct timespec *queue_entrance);

int wait_to_be_served(void);

//int enter_random_queue(client_t *cl, client_in_queue *cl_in_q, grocerystore_t *gs, gs_state *store_state);

#endif //CLIENT_H
