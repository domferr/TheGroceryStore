#include "manager.h"
#include "utils.h"
#include "queue.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>

#define DEBUG_MANAGER

static int close_random_cashier(cashier_sync **ca_sync, size_t k);
static int activate_cashier(cashier_sync **ca_sync, size_t k);
static int run_cashiers_algorithm(manager_arr_t *marr, int s1, int s2);

void *manager_fun(void *args) {
    int err;
    gs_state store_state;
    manager_args *ma = (manager_args*) args;
    grocerystore_t *gs = ma->gs;
    queue_t *queue = (ma->queue)->queue;

    pthread_mutex_t *mutex = &((ma->queue)->mutex);
    pthread_cond_t *wait = &((ma->queue)->waiting);

    NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
    while (ISOPEN(store_state)) {
        PTH(err, pthread_mutex_lock(mutex), perror("manager lock"); return NULL)
        while(ISOPEN(store_state) && queue->size == 0) {
            PTH(err, pthread_cond_wait(wait, mutex), perror("manager cond waiting"); return NULL)
            PTH(err, pthread_mutex_unlock(mutex), perror("manager unlock"); return NULL)
            NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
            PTH(err, pthread_mutex_lock(mutex), perror("manager lock"); return NULL)
        }
        MINUS1(handle_notification(ma->queue, ma->marr), perror("handle_notification"); return NULL)
        PTH(err, pthread_mutex_unlock(mutex), perror("manager unlock"); return NULL)
        MINUS1(run_cashiers_algorithm(ma->marr, ma->s1, ma->s2), perror("run cashiers algorithm"); return NULL)
        NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
    }
#ifdef DEBUG_MANAGER
    printf("Manager: termino\n");
#endif

    destroy_manager_arr(ma->marr);
    free(ma);
    return 0;
}

static int close_random_cashier(cashier_sync **ca_sync, size_t k) {
    unsigned int seed = 1000;
    int found = 0, err;
    size_t i = 0;
    cashier_sync *ca;
    while(!found) {
        i = RANDOM(seed, 0, k);
        ca = ca_sync[i];
        PTH(err, pthread_mutex_lock(&(ca->mutex)), return -1)
        if (ca->state == active) {
            ca->state = sleeping;
            found = 1;
#ifdef DEBUG_MANAGER
            printf("Manager: chiudo la cassa %ld\n", i);
#endif
            //Sveglio il cassiere se dovesse essere in attesa sulla coda vuota
            PTH(err, pthread_cond_signal(&(ca->noclients)), return 0)
        }
        PTH(err, pthread_mutex_unlock(&(ca->mutex)), return -1)
    }
    return i;
}

static int activate_cashier(cashier_sync **ca_sync, size_t k) {
    size_t i = 0;
    int found = 0, err;
    cashier_sync *ca;
    while (i<k && !found) {
        ca = ca_sync[i];
        PTH(err, pthread_mutex_lock(&(ca->mutex)), return -1)
        if (ca->state == sleeping) {
            ca->state = active;
            found = 1;
#ifdef DEBUG_MANAGER
            printf("Manager: apro la cassa %ld\n", i);
#endif
            //Sveglio il cassiere
            PTH(err, pthread_cond_signal(&(ca->paused)), return 0)
        }
        PTH(err, pthread_mutex_unlock(&(ca->mutex)), return -1)
        i++;
    }
    return 0;
}

static int run_cashiers_algorithm(manager_arr_t *marr, int s1, int s2) {
    int found = 0;
    size_t i = 0;
    int *counters = marr->counters;
    //Cerca se ci sono almeno S1 casse che hanno almeno un cliente
    if (marr->active_cashiers > 1) {    //Se c'è almeno una cassa aperta, valuto se chiuderne qualcuna
        while (i < marr->size && found < s1) {
            if (counters[i] >= 1)
                found++;
            i++;
        }
        if (i < marr->size) {
            i = close_random_cashier(marr->ca_sync, marr->size);
            MINUS1(i, return -1)
            counters[i] = 0;
            marr->active_cashiers -= 1;
        }
    }

    found = 0;
    i = 0;
    //Cerca se c'è almeno una cassa che ha almeno S2 clienti in coda
    if (marr->active_cashiers < marr->size) {    //Se ci sono k casse attive allora sono già tutte attive
        while (i < marr->size && !found) {
            if (counters[i] >= s2)
                found = 1;
            i++;
        }
        if (found) {
            MINUS1(activate_cashier(marr->ca_sync, marr->size), return -1);
            marr->active_cashiers += 1;
        }
    }
    return 0;
}

manager_queue *alloc_manager_queue(void) {
    int err;
    manager_queue *maqueue = (manager_queue*) malloc(sizeof(manager_queue));
    EQNULL(maqueue, return NULL);
    PTH(err, pthread_mutex_init(&((maqueue)->mutex), NULL), return NULL)
    PTH(err, pthread_cond_init(&((maqueue)->waiting), NULL), return NULL)

    (maqueue)->queue = queue_create();
    EQNULL((maqueue)->queue, return NULL);

    return maqueue;
}

manager_args *alloc_manager(grocerystore_t *gs, int s1, int s2, int ka, manager_queue *mq, cashier_t **cashiers, size_t ncash) {
    manager_args *ma = (manager_args*) malloc(sizeof(manager_args));
    EQNULL(ma, return NULL)

    ma->gs = gs;
    ma->s1 = s1;
    ma->s2 = s2;
    ma->queue = mq;
    ma->marr = create_manager_arr(cashiers, ncash, ka);
    EQNULL(ma->marr, return NULL)
    return ma;
}

int wakeup_manager(manager_queue *mqueue) {
    int err;
    PTH(err, pthread_mutex_lock(&(mqueue->mutex)), return -1)
    PTH(err, pthread_cond_signal(&(mqueue->waiting)), return -1)
    PTH(err, pthread_mutex_unlock(&(mqueue->mutex)), return -1)
    return 0;
}

int handle_notification(manager_queue *mqueue, manager_arr_t *marr) {
    queue_t *queue = mqueue->queue;
    notification_data *data = pop(queue);
    if (data != NULL) {
#ifdef DEBUG_MANAGER
        //printf("Manager: nella cassa %ld ci sono %d clienti\n", data->id, data->clients_in_queue);
#endif
        (marr->counters)[data->id] = data->clients_in_queue;
        free(data);
        return 1;
    }
    return 0;
}

int destroy_manager_arr(manager_arr_t *arr) {
    free(arr->counters);
    free(arr->ca_sync);
    free(arr);
    return 0;
}

manager_arr_t *create_manager_arr(cashier_t **cashiers, size_t size, size_t active_cashiers) {
    size_t i;
    manager_arr_t *marr = (manager_arr_t*) malloc(sizeof(manager_arr_t));
    EQNULL(marr, return NULL);
    marr->size = size;
    marr->ca_sync = (cashier_sync**) malloc(sizeof(cashier_sync*)*size);
    EQNULL(marr->ca_sync, return NULL)
    marr->counters = (int*) malloc(sizeof(int)*size);
    EQNULL(marr->counters, return NULL)
    for (i = 0; i < size; i++) {
        (marr->ca_sync)[i] = (cashiers[i])->ca_sync;
        (marr->counters)[i] = 0;
    }
    marr->active_cashiers = active_cashiers;
    return marr;
}

int destroy_manager_queue(manager_queue *mqueue) {
    int err;
    MINUS1(queue_destroy(mqueue->queue), return -1)
    PTH(err, pthread_mutex_destroy(&(mqueue->mutex)), return -1)
    PTH(err, pthread_cond_destroy(&(mqueue->waiting)), return -1)

    free(mqueue);
    return 0;
}