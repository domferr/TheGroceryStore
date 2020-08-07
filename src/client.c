#define _POSIX_C_SOURCE 200809L

#include "../include/client.h"
#include "../include/store.h"
#include "../include/config.h"
#include "../include/utils.h"
#include "../include/stats.h"
#include "../include/client_in_queue.h"
#include <stdlib.h>
#include <stdio.h>

#define DEBUGCLIENT

client_t *alloc_client(size_t id, store_t *store, cassiere_t **casse, int t, int p, int s, int k) {
    int err;
    client_t *client = (client_t*) malloc(sizeof(client_t));
    EQNULL(client, return NULL)

    client->store = store;
    client->id = id;
    client->casse = casse;
    client->k = k;
    client->t = t;
    client->p = p;
    client->s = s;

    client->can_exit = -1;
    PTH(err, pthread_mutex_init(&(client->mutex), NULL), return NULL)
    PTH(err, pthread_cond_init(&(client->exit_permission), NULL), return NULL)

    return client;
}

int client_destroy(client_t *client) {
    int err;
    PTH(err, pthread_mutex_destroy(&(client->mutex)), return -1)
    PTH(err, pthread_cond_destroy(&(client->exit_permission)), return -1)
    free(client);
    return 0;
}

void *client_thread_fun(void *args) {
    client_t *cl = (client_t *) args;
    int random_time, client_id, products, served, entered_queue;
    struct timespec store_entrance, queue_entrance;
    unsigned int seed = cl->id;
    store_state st_state;
    queue_t *stats;
    client_in_queue *clq;
    client_stats *this_client_stats;

    EQNULL(stats = queue_create(), perror("queue_create()"); return NULL)
    EQNULL(clq = alloc_client_in_queue(cl->id, &(cl->mutex)), perror("alloc_client_in_queue()"); return NULL)

    NOTZERO(get_store_state(&st_state), perror("get store state"); return NULL)
    while (ISOPEN(st_state)) {
        client_id = enter_store(cl->store);
        if (client_id) {
            MINUS1(clock_gettime(CLOCK_MONOTONIC, &store_entrance), perror("clock_gettime"); return NULL)
            EQNULL(this_client_stats = new_client_stats(client_id), perror("new client stats"); return NULL)
            random_time = RANDOM(seed, MIN_T, cl->t);
            NOTZERO(msleep(random_time), perror("msleep"); return NULL)
            products = RANDOM(seed, 0, cl->p);
            this_client_stats->products = products;
            NOTZERO(get_store_state(&st_state), perror("get store state"); return NULL)
#ifdef DEBUGCLIENT
            printf("Cliente %d: voglio acquistare %d prodotti\n", client_id, products);
#endif
            if (products == 0) {
#ifdef DEBUGCLIENT
                printf("Thread cliente %ld: Chiedo permesso di uscita\n", cl->id);
#endif
                //Se il supermercato è ancora aperto, chiedo il permesso di uscire ad aspetto che mi venga dato
                if (ISOPEN(st_state)) {
                    MINUS1(ask_exit_permission(cl->id), perror("ask exit permission"); return NULL)
                    MINUS1(wait_permission(cl), perror("wait permission"); return NULL)
                }
            } else {
#ifdef DEBUGCLIENT
                printf("Thread cliente %ld: mi metto in coda in una delle casse\n", cl->id);
#endif
                clq->products = products;
                clq->served = 0;
                served = 0;
                while(st_state != closed_fast_state && !served) {
                    if (st_state == open_state)
                        MINUS1(enter_best_queue(cl, clq, &queue_entrance), perror("enter best queue"); return NULL)
                    MINUS1(served = wait_to_be_served(cl->s, clq, &st_state), perror("waiting to be served"); return NULL)
                }
                if (served) {
                    MINUS1(this_client_stats->time_in_queue = elapsed_time(&queue_entrance), perror("elapsed ms from"); return NULL)
                }
            }
            MINUS1(leave_store(cl->store), perror("exit store"); exit(EXIT_FAILURE))
            MINUS1(this_client_stats->time_in_store = elapsed_time(&store_entrance), perror("elapsed ms from"); return NULL)
            MINUS1(push(stats, this_client_stats), perror("push in queue"); return NULL)
#ifdef DEBUGCLIENT
            printf("Thread cliente %ld: sono uscito dal supermercato\n", cl->id);
#endif
        }
        NOTZERO(get_store_state(&st_state), perror("get store state"); return NULL)
    }
#ifdef DEBUGCLIENT
    printf("Thread cliente %ld: termino\n", cl->id);
#endif
    MINUS1(destroy_client_in_queue(clq), perror("destroy client in queue"); exit(EXIT_FAILURE))

    return stats;
}

int set_exit_permission(client_t *client, int can_exit) {
    int err;
    PTH(err, pthread_mutex_lock(&(client->mutex)), return -1)
    client->can_exit = can_exit;
    PTH(err, pthread_cond_signal(&(client->exit_permission)), return -1)
    PTH(err, pthread_mutex_unlock(&(client->mutex)), return -1)
    return 0;
}

int wait_permission(client_t *cl) {
    int err;
    cl->can_exit = 0;
    PTH(err, pthread_mutex_lock(&(cl->mutex)), return -1)
    while (cl->can_exit == 0) {
        PTH(err, pthread_cond_wait(&(cl->exit_permission), &(cl->mutex)), return -1)
    }
    PTH(err, pthread_mutex_unlock(&(cl->mutex)), return -1)
    return 0;
}

int enter_best_queue(client_t *cl, client_in_queue *clq, struct timespec *queue_entrance) {
    int err, i = 0, entered = 0;
    cassiere_t *ca;
    MINUS1(clock_gettime(CLOCK_MONOTONIC, queue_entrance), return -1)
    while (!entered) {
        ca = cl->casse[i];
        PTH(err, pthread_mutex_lock(&(ca->mutex)), return -1)
        if (ca->state == cassa_open_state) {
            MINUS1(push(ca->queue, clq), return -1)
            entered = 1;
        }
        PTH(err, pthread_mutex_unlock(&(ca->mutex)), return -1)
    }

    return 0;
}

int wait_to_be_served(int s, client_in_queue *clq, store_state *st_state) {
    int err, timeout = 0, served;
    struct timespec max_wait = {MS_TO_SEC(s), MS_TO_NANOSEC(s)};

    PTH(err, pthread_mutex_lock(clq->mutex), return -1)
    while (ISOPEN(*st_state) && !timeout && clq->served == 0) {
        PTH(err, pthread_cond_wait(&(clq->waiting), clq->mutex), return -1)
        PTH(err, pthread_mutex_unlock(clq->mutex), return -1)
        NOTZERO(get_store_state(st_state), return -1)
        PTH(err, pthread_mutex_lock(clq->mutex), return -1)
        /*
        if ((err = pthread_cond_timedwait(&(clq->waiting), clq->mutex, &max_wait)) == ETIMEDOUT) {
            timeout = 1;
        } else {
            errno = err;
            return -1;
        }*/
    }
    served = clq->served;
    PTH(err, pthread_mutex_unlock(clq->mutex), return -1)

    return served;
}