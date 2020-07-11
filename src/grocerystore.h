#ifndef GROCERYSTORE_H
#define GROCERYSTORE_H

#define _POSIX_C_SOURCE 200809L
#include <pthread.h>

#define ISOPEN(st) st == open
#define ISCLOSED(st) st != open

typedef enum {
    open, closed, closed_fast, gserror
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
void grocerystore_destroy(grocerystore_t *gs);  //TODO detroy grocery store
gs_state enter_store(grocerystore_t *gs, size_t id);
int exit_store(grocerystore_t *gs);
gs_state doBusiness(grocerystore_t *gs, int c, int e);

#endif //GROCERYSTORE_H