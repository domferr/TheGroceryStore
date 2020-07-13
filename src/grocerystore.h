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
    int clients_inside;
    pthread_cond_t exit;
    pthread_cond_t entrance;
    int can_enter;
} grocerystore_t;

grocerystore_t *grocerystore_create(size_t e);
void grocerystore_destroy(grocerystore_t *gs);
int enter_store(grocerystore_t *gs);
int exit_store(grocerystore_t *gs);
int manage_entrance(grocerystore_t *gs, gs_state *state, int c, int e);
int get_store_state(grocerystore_t *gs, gs_state *state);

#endif //GROCERYSTORE_H