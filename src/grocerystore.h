#ifndef GROCERYSTORE_H
#define GROCERYSTORE_H

#define _POSIX_C_SOURCE 200809L
#include <pthread.h>

#define ISOPEN(st) st == open
#define ISCLOSED(st) st != open

typedef enum {
    open, closed, closed_fast
} gs_state;

typedef struct {
    pthread_mutex_t mutex;
    gs_state state;
    size_t clients_inside;
    pthread_cond_t exit;
    pthread_cond_t entrance;
    size_t can_enter;
} grocerystore_t;

grocerystore_t *grocerystore_create(size_t e);
void grocerystore_destroy(grocerystore_t *gs);
gs_state enter_store(grocerystore_t *gs, size_t id);
void exit_store(grocerystore_t *gs);
gs_state doBusiness(grocerystore_t *gs, size_t c, size_t e);

#endif //GROCERYSTORE_H