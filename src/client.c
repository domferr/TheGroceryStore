#define _POSIX_C_SOURCE 200809L

#include "../include/client.h"
#include "../include/store.h"
#include "../include/config.h"
#include "../include/af_unix_conn.h"
#include "../include/utils.h"
#include <stdlib.h>
#include <stdio.h>

#define DEBUGCLIENT

client_t *alloc_client(size_t id, store_t *store, int t, int p, int s, int k, int fd, pthread_mutex_t *fd_mtx) {
    int err;
    client_t *client = (client_t*) malloc(sizeof(client_t));
    EQNULL(client, return NULL)

    client->store = store;
    client->id = id;
    //client->cashiers = cashiers;  TODO add cashiers
    client->k = k;
    client->t = t;
    client->p = p;
    client->s = s;
    client->fd = fd;
    client->fd_mtx = fd_mtx;

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
    int err, random_time, client_id, products, chosen_queue, served;
    unsigned int seed = cl->id;
    store_state st_state;
    struct timespec store_entrance, store_exit, queue_entrance, queue_exit;
    store_t *store = (store_t *) cl->store;

    NOTZERO(get_store_state(store, &st_state), perror("get store state"); return NULL)
    while (ISOPEN(st_state)) {
        client_id = enter_store(store);
        if (client_id) {
            MINUS1(clock_gettime(CLOCK_MONOTONIC, &store_entrance), perror("clock_gettime"); return NULL)
            random_time = RANDOM(seed, MIN_T, cl->t);
            NOTZERO(msleep(random_time), perror("msleep"); return NULL)
            products = RANDOM(seed, 0, cl->p);
            NOTZERO(get_store_state(store, &st_state), perror("get store state"); return NULL)
#ifdef DEBUGCLIENT
            printf("Thread cliente %ld: voglio acquistare %d prodotti\n", cl->id, products);
#endif
            if (products == 0) {
#ifdef DEBUGCLIENT
                printf("Thread cliente %ld: Chiedo permesso di uscita\n", cl->id);
#endif
                //Se il supermercato è ancora aperto, chiedo il permesso di uscire ad aspetto che mi venga dato
                if (ISOPEN(st_state)) {
                    PTH(err, pthread_mutex_lock(cl->fd_mtx), perror("lock"); return NULL)
                    MINUS1(ask_exit_permission(cl->fd, cl->id), perror("ask exit permission"); return NULL)
                    PTH(err, pthread_mutex_unlock(cl->fd_mtx), perror("unlock"); return NULL)
                    MINUS1(wait_permission(cl), perror("wait permission"); return NULL)
                }
            } else {
#ifdef DEBUGCLIENT
                printf("Thread cliente %ld: mi metto in coda in una delle casse\n", cl->id);
#endif
                served = 0;
                while(st_state != closed_fast_state && !served) {
                    MINUS1(chosen_queue = enter_best_queue(&queue_entrance), perror("enter best queue"); return NULL)
                    MINUS1(served = wait_to_be_served(), perror("waiting to be served"); return NULL)
                    NOTZERO(get_store_state(store, &st_state), perror("get store state"); return NULL)
                }
                if (served) {
                    MINUS1(clock_gettime(CLOCK_MONOTONIC, &queue_exit), perror("clock_gettime"); return NULL)
                    //current_client_stats->time_in_queue = get_elapsed_milliseconds(queue_entrance, queue_exit);
                    //current_client_stats->products = cl_in_q->products;
                }
            }
            MINUS1(exit_store(store), perror("exit store"); exit(EXIT_FAILURE))
            MINUS1(clock_gettime(CLOCK_MONOTONIC, &store_exit), perror("clock_gettime"); return NULL)
            //current_client_stats->time_in_store = get_elapsed_milliseconds(store_entrance, store_exit);
#ifdef DEBUGCLIENT
            printf("Thread cliente %ld: sono uscito dal supermercato\n", cl->id);
#endif
        }
        NOTZERO(get_store_state(store, &st_state), perror("get store state"); return NULL)
    }
#ifdef DEBUGCLIENT
    printf("Thread cliente %ld: termino\n", cl->id);
#endif
    return 0;
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

int enter_best_queue(struct timespec *queue_entrance) {
    int chosen = 0;
    MINUS1(clock_gettime(CLOCK_MONOTONIC, queue_entrance), return -1)
    return chosen;
}

int wait_to_be_served(void) {
    return 1;
}
        /*switch(internal_state) {
            case on_entrance:
                client_id = enter_store(store);
                MINUS1(client_id, perror("Impossibile entrare nel supermercato"); return NULL)
                if (client_id) {    //Se sono entrato nel supermercato
                    internal_state = take_products;
                    MINUS1(clock_gettime(CLOCK_MONOTONIC, &store_entrance), perror("clock_gettime"); return NULL)
                    current_client_stats = alloc_client_stats(client_id);
                    EQNULL(current_client_stats, perror("alloc client stats"); return NULL)
                    MINUS1(push(thread_stats->queue, current_client_stats), perror("push into stats queue"); return NULL)
                    run = 1;
                } else {    //Devo terminare perchè ero all'ingresso ma il supermercato sta chiudendo
                    run = 0;
                }
                break;
            case take_products:
                cl_in_q->products = RANDOM(seed, 0, cl->p);
                random_time = RANDOM(seed, MIN_T, cl->t);

#ifdef DEBUGCLIENT
                printf("Thread cliente %ld: Entro nel supermercato. Prendo %d prodotti in %dms\n", cl->id, cl_in_q->products, random_time);
#endif
                NOTZERO(msleep(random_time), perror("msleep"); return NULL)
                NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
                //Sono dentro al supermercato quindi esco solo se lo stato è closed_fast (SIGQUIT)
                //oppure se non ho nessun prodotto da acquistare
                if (cl_in_q->products == 0 || store_state == closed_fast) {
                    internal_state = on_exit;
                } else {    //Altrimenti mi accodo ad una cassa
                    internal_state = enter_queue;
                }
                break;
            case enter_queue:
                cl_in_q->status = waiting;
                //Entro in una coda di una cassa random aperta
                err = enter_random_queue(cl, cl_in_q, gs, &store_state);
                MINUS1(err, perror("client enter random queue"); return NULL)
                MINUS1(clock_gettime(CLOCK_MONOTONIC, &queue_entrance), perror("clock_gettime"); return NULL)
                //Fino a quando non sono stato servito oppure lo store è aperto o chiuso tramite SIGHUP
                PTH(err, pthread_mutex_lock(&(cl_in_q)->mutex), perror("client mutex lock"); return NULL)
                while (store_state != closed_fast && cl_in_q->status == waiting) {
                    PTH(err, pthread_cond_wait(&(cl_in_q->waiting), (&(cl_in_q->mutex))), perror("client cond wait"); return NULL)
                    PTH(err, pthread_mutex_unlock(&(cl_in_q->mutex)), perror("client mutex unlock"); return NULL)
                    //Aggiorno lo stato del store perchè devo terminare se lo stato è closed_fast
                    NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
                    PTH(err, pthread_mutex_lock(&(cl_in_q)->mutex), perror("client mutex lock"); return NULL)
                }
                PTH(err, pthread_mutex_unlock(&(cl_in_q->mutex)), perror("client mutex unlock"); return NULL)

                //Se sono stato servito, esco dal supermercato. Se invece non sono stato servito,
                //allora la cassa potrebbe essere chiusa. In questo caso rimango nello stesso stato.
                //Se invece non sono stato servito perchè il supermercato chiude, allora esco dal supermercato.
                if (cl_in_q->status == store_closure) {
                    internal_state = on_exit;
                } else if (cl_in_q->status == done || cl_in_q->status == store_closure) {
                    MINUS1(clock_gettime(CLOCK_MONOTONIC, &queue_exit), perror("clock_gettime"); return NULL)
                    current_client_stats->time_in_queue = get_elapsed_milliseconds(queue_entrance, queue_exit);
                    current_client_stats->products = cl_in_q->products;
                    internal_state = on_exit;
                }
                break;
            case on_exit:
                if (cl_in_q->products == 0) {

                }
#ifdef DEBUGCLIENT
                    printf("Thread cliente %ld: esco dal supermercato\n", cl->id);
#endif
                NOTZERO(exit_store(gs, &store_state), perror("exit store"); return NULL)
                MINUS1(clock_gettime(CLOCK_MONOTONIC, &store_exit), perror("clock_gettime"); return NULL)
                current_client_stats->time_in_store = get_elapsed_milliseconds(store_entrance, store_exit);
                //Rendo il thread riutilizzabile se il supermercato è aperto
                internal_state = on_entrance;
                run = store_state != closed_fast;
                break;
        }
    }
#ifdef DEBUGCLIENT
    printf("Thread cliente %ld terminato\n", cl->id);
#endif

    MINUS1(destroy_client_in_queue(cl_in_q), perror("destroy client in queue"); return NULL)
    free(cl);
    return thread_stats;
}*/