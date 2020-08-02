#define _POSIX_C_SOURCE 200809L
#include "../include/client.h"
#include "../include/store.h"
#include "../include/utils.h"
#include "../include/config.h"
#include "../include/af_unix_conn.h"
#include "../include/scfiles.h"
#include <stdlib.h>
#include <stdio.h>

//#define DEBUGCLIENT

static int ask_exit_permission(int fd, int client_id) {
    msg_header_t msg_hdr = head_ask_exit;
    MINUS1(writen(fd, &msg_hdr, sizeof(msg_header_t)), return -1)
    MINUS1(writen(fd, &client_id, sizeof(int)), return -1)
    return 0;
}

client_t *alloc_client(size_t id, store_t *store, int t, int p, int s, int *shared_pipe, size_t no_of_cashiers) {
    client_t *client = (client_t*) malloc(sizeof(client_t));
    EQNULL(client, return NULL)

    client->store = store;
    client->id = id;
    //client->cashiers = cashiers;  TODO add cashiers
    client->no_of_cashiers = no_of_cashiers;
    client->t = t;
    client->p = p;
    client->s = s;
    client->shared_pipe = shared_pipe;

    return client;
}


void *client_thread_fun(void *args) {
    client_t *cl = (client_t *) args;
    int err, random_time, client_id, products;
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
#ifdef DEBUGCLIENT
            printf("Thread cliente %ld: voglio acquistare %d prodotti\n", cl->id, products);
#endif
            if (products == 0) {
#ifdef DEBUGCLIENT
                printf("Thread cliente %ld: Chiedo permesso di uscita\n", cl->id);
#endif
                MINUS1(ask_exit_permission(cl->shared_pipe[1], client_id), perror("ask exit permission"); exit(EXIT_FAILURE))
            } else {
#ifdef DEBUGCLIENT
                printf("Thread cliente %ld: mi metto in coda in una delle casse\n", cl->id);
#endif
                /*chosen_queue = enter_best_queue(&queue_entrance);
                while(!should_exit && !served) {
                    wait_to_be_served(chosen_queue);
                    if (errno == ETIMEDOUT && store_state != closed_fast_state)
                        chosen_queue = enter_best_queue(&queue_entrance);
                }*/
                /*MINUS1(clock_gettime(CLOCK_MONOTONIC, &queue_exit), perror("clock_gettime"); return NULL)
                current_client_stats->time_in_queue = get_elapsed_milliseconds(queue_entrance, queue_exit);
                current_client_stats->products = cl_in_q->products;*/
            }
            MINUS1(exit_store(store), perror("exit store"); exit(EXIT_FAILURE))
            /*MINUS1(clock_gettime(CLOCK_MONOTONIC, &store_exit), perror("clock_gettime"); return NULL)
            current_client_stats->time_in_store = get_elapsed_milliseconds(store_entrance, store_exit);*/
#ifdef DEBUGCLIENT
            printf("Thread cliente %ld: sono uscito dal supermercato\n", cl->id);
#endif
        }
        NOTZERO(get_store_state(store, &st_state), perror("get store state"); return NULL)
    }
#ifdef DEBUGCLIENT
    printf("Thread cliente %ld: termino\n", cl->id);
#endif
    free(cl);
    return 0;
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
                    //TODO Aspetto il permesso del direttore e poi esco dal supermercato
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