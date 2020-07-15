#include "manager.h"
#include "cashier.h"
#include "utils.h"
#include "queue.h"
#include <stdlib.h>

typedef struct {
    size_t size;
    cashier_sync **ca_sync;
    int **counters;
} manager_arr_t;

static manager_arr_t *create_manager_arr(cashier_t **cashiers, size_t size);
static int destroy_manager_arr(manager_arr_t *arr);

void *manager_fun(void *args) {
    int err;
    gs_state store_state;
    notification_data *data;

    manager_args *ma = (manager_args*) args;
    manager_arr_t *marr = create_manager_arr(NULL, 0);
    pthread_mutex_t *mutex = &((ma->queue)->mutex);
    pthread_cond_t *wait = &((ma->queue)->waiting);
    queue_t *queue = (ma->queue)->queue;
    grocerystore_t *gs = ma->gs;

    NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
    while (ISOPEN(store_state)) {
        PTH(err, pthread_mutex_lock(mutex), perror("manager lock"); return NULL)
        while(ISOPEN(store_state) && queue->size == 0) {
            PTH(err, pthread_cond_wait(wait, mutex), perror("manager cond waiting"); return NULL)
            PTH(err, pthread_mutex_unlock(mutex), perror("manager unlock"); return NULL)
            NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
            PTH(err, pthread_mutex_lock(mutex), perror("manager lock"); return NULL)
        }
        MINUS1(handle_notification(ma->queue), perror("handle_notification"); return NULL)
        PTH(err, pthread_mutex_unlock(mutex), perror("manager unlock"); return NULL)

        NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
    }
    destroy_manager_arr(marr);
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
    return ma;
}

int wakeup_manager(manager_queue *mqueue) {
    int err;
    PTH(err, pthread_mutex_lock(&(mqueue->mutex)), return -1)
    PTH(err, pthread_cond_signal(&(mqueue->waiting)), return -1)
    PTH(err, pthread_mutex_unlock(&(mqueue->mutex)), return -1)
    return 0;
}

int notify_manager(manager_queue *mq, size_t cashier_id, int clients_in_queue) {
    int err;
    queue_t *queue = mq->queue;

    notification_data *data = (notification_data*) malloc(sizeof(notification_data));
    EQNULL(data, return -1);
    data->id = cashier_id;
    data->clients_in_queue = clients_in_queue;

    PTH(err, pthread_mutex_lock(&(mq->mutex)), return -1)
    MINUS1(push(queue, data), return -1)
    PTH(err, pthread_cond_signal(&(mq->waiting)), return -1)
    PTH(err, pthread_mutex_unlock(&(mq->mutex)), return -1)
    return 0;
}

int handle_notification(manager_queue *mqueue) {
    queue_t *queue = mqueue->queue;
    notification_data *data = pop(queue);
    if (data != NULL) {

        free(data);
        return 1;
    }
    return 0;
}

int destroy_manager(manager_args *ma) {
    int err;
    manager_queue *mqueue = ma->queue;
    MINUS1(queue_destroy(mqueue->queue), return -1)
    PTH(err, pthread_mutex_destroy(&(mqueue->mutex)), return -1)
    PTH(err, pthread_cond_destroy(&(mqueue->waiting)), return -1)
    free(mqueue);
    free(ma);
    return 0;
}

static manager_arr_t *create_manager_arr(cashier_t **cashiers, size_t size) {
    size_t i;
    manager_arr_t *marr = (manager_arr_t*) malloc(sizeof(manager_arr_t));
    EQNULL(marr, return NULL);
    marr->size = size;
    marr->ca_sync = (cashier_sync**) malloc(sizeof(cashier_sync*)*size);
    EQNULL(marr->ca_sync, return NULL)
    for (i = 0; i < size; i++) {
        (marr->ca_sync)[i] = (cashiers[i])->ca_sync;
    }
    marr->counters = (int**) malloc(sizeof(int*)*size);
    EQNULL(marr->counters, return NULL)
    return marr;
}

static int destroy_manager_arr(manager_arr_t *arr) {
    free(arr->counters);
    free(arr->ca_sync);
    free(arr);
    return 0;
}
