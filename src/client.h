#ifndef CLIENT_H
#define CLIENT_H
#include "grocerystore.h"

typedef struct {
    size_t id;
    grocerystore_t *gs;
    int t;  //tempo massimo per acquistare prima di mettersi in una coda
    int p;  //numero massimo di prodotti che acquista
} client_t;

void *client_fun(void *args);
client_t *alloc_client(size_t id, grocerystore_t *gs, int t, int p);

#endif //CLIENT_H
