#include "manager.h"
#include "utils.h"
#include "queue.h"
#include "notifier.h"
#include <stdlib.h>

static int handle_notification(manager_args *ma, notifier_data *data);
static manager_arr_t *create_manager_arr(size_t size);
static int destroy_manager_arr(manager_arr_t *arr);

void *manager_fun(void *args) {
    int err;
    manager_args *ma = (manager_args*) args;
    manager_arr_t *marr = ma->marr;
    pthread_mutex_t *mutex = &((ma->queue)->mutex);
    pthread_cond_t *wait = &((ma->queue)->waiting);
    queue_t *queue = (ma->queue)->queue;
    grocerystore_t *gs = ma->gs;
    gs_state store_state;
    notifier_data *notif;

    NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)

    while (ISOPEN(store_state)) {
        PTH(err, pthread_mutex_lock(mutex), perror("manager lock"); return NULL)
        while(ISOPEN(store_state) && queue->size == 0) {
            PTH(err, pthread_cond_wait(wait, mutex), perror("manager cond waiting"); return NULL)
            PTH(err, pthread_mutex_unlock(mutex), perror("manager unlock"); return NULL)
            NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
            PTH(err, pthread_mutex_lock(mutex), perror("manager lock"); return NULL)
        }
        notif = pop(queue);
        if (notif != NULL) {
            MINUS1(handle_notification(ma, notif), perror("handle_notification"); return NULL)

        }
        PTH(err, pthread_mutex_unlock(mutex), perror("manager unlock"); return NULL)

        NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
    }
    destroy_manager(ma);
    return 0;
}

manager_args *alloc_manager(grocerystore_t *gs, int s1, int s2, cashier_t **cashiers, size_t no_of_cashiers) {
    int err;
    manager_args *ma = (manager_args*) malloc(sizeof(manager_args));
    EQNULL(ma, return NULL);
    ma->queue = (manager_queue*) malloc(sizeof(manager_queue));
    EQNULL(ma->queue, return NULL);
    PTH(err, pthread_mutex_init(&((ma->queue)->mutex), NULL), return NULL)
    PTH(err, pthread_cond_init(&((ma->queue)->waiting), NULL), return NULL)

    (ma->queue)->queue = queue_create();
    EQNULL((ma->queue)->queue, return NULL);

    ma->gs = gs;
    ma->s1 = s1;
    ma->s2 = s2;
    ma->cashiers = cashiers;
    ma->marr = create_manager_arr(no_of_cashiers);
    EQNULL(ma->marr, return NULL);
    return ma;
}

int destroy_manager(manager_args *ma) {
    int err;
    manager_queue *mqueue = ma->queue;
    MINUS1(queue_destroy(mqueue->queue), return -1);
    PTH(err, pthread_mutex_destroy(&(mqueue->mutex)), return -1);
    PTH(err, pthread_cond_destroy(&(mqueue->waiting)), return -1);
    MINUS1(destroy_manager_arr(ma->marr), return -1);
    free(mqueue);
    free(ma);
    return 0;
}

static manager_arr_t *create_manager_arr(size_t size) {
    manager_arr_t *marr = (manager_arr_t*) malloc(sizeof(manager_arr_t));
    EQNULL(marr, return NULL);
    marr->size = size;
    return marr;
}

static int destroy_manager_arr(manager_arr_t *arr) {
    free(arr);
    return 0;
}

int wakeup_manager(manager_queue *mqueue) {
    int err;
    PTH(err, pthread_mutex_lock(&(mqueue->mutex)), return -1)
    PTH(err, pthread_cond_signal(&(mqueue->waiting)), return -1)
    PTH(err, pthread_mutex_unlock(&(mqueue->mutex)), return -1)
    return 0;
}

static int handle_notification(manager_args *ma, notifier_data *data) {

    free(data);
    return 0;
}