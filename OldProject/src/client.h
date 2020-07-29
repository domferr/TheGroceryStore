#ifndef CLIENT_H
#define CLIENT_H

#include "grocerystore.h"
#include "cashier.h"
#include "client_in_queue.h"

typedef struct {
    size_t id;
    grocerystore_t *gs;
    cashier_t **cashiers;
    size_t no_of_cashiers;
    int t;  //tempo massimo per acquistare prima di mettersi in una coda
    int p;  //numero massimo di prodotti che acquista
} client_t;

void *client_fun(void *args);
client_t *alloc_client(size_t id, grocerystore_t *gs, int t, int p, cashier_t **cashiers, size_t no_of_cashiers);

int enter_random_queue(client_t *cl, client_in_queue *cl_in_q, grocerystore_t *gs, gs_state *store_state);

#endif //CLIENT_H
