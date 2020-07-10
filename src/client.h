#ifndef CLIENT_H
#define CLIENT_H
#include "grocerystore.h"

typedef struct {
    size_t id;
    grocerystore_t *gs;
} client_t;

void *client_fun(void *args);

#endif //CLIENT_H
