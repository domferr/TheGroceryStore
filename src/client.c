#define _POSIX_C_SOURCE 200809L

#define DEBUGGING 0
#include "../include/client.h"
#include "../include/store.h"
#include "../include/config.h"
#include "../include/utils.h"
#include "../include/stats.h"
#include "../include/client_in_queue.h"
#include "../include/cassa_queue.h"
#include <stdlib.h>
#include <stdio.h>

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
    client->can_exit = 0;
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

/**
 * Prende un certo numero di prodotti in un certo periodo il tempo. I prodotti sono maggiori o uguali a zero e minori
 * del parametro p. Il tempo necessario è maggiore o uguale a MIN_T e minore del parametro t.
 *
 * @param cl struttura dati del cliente
 * @return numero di prodotti in caso di successo, -1 altrimenti ed imposta errno
 */
static int get_products(client_t *cl);

void *client_thread_fun(void *args) {
    client_t *cl = (client_t *) args;
    int client_id, err, cost, change_queue = 0, queue_counter = 0;
    long time_in_store, time_in_queue;
    struct timespec store_entrance, queue_entrance, waitingtime = {MS_TO_SEC(cl->s), MS_TO_NANOSEC(cl->s)};
    store_state st_state;
    queue_t *stats;
    client_in_queue_t *clq;
    cassiere_t *cassiere = NULL, *best_cassiere;

    EQNULL(stats = queue_create(), perror("queue_create"); return NULL)
    EQNULL(clq = alloc_client_in_queue(), perror("alloc_client_in_queue"); return NULL)

    NOTZERO(get_store_state(&st_state), perror("get store state"); return NULL)
    while (ISOPEN(st_state)) {
        //Resetto i contatori delle statistiche
        queue_counter = 0;
        //Entro nel supermercato. Chiamata bloccante. Il controllo ritorna se entro nel supermercato o se esso viene chiuso
        client_id = enter_store(cl->store);
        if (client_id) {
            MINUS1(clock_gettime(CLOCK_MONOTONIC, &store_entrance), perror("clock_gettime"); return NULL)
            DEBUG("[Thread Cliente %ld] Cliente %d: sono entrato nel supermercato\n", cl->id, client_id)
            MINUS1(clq->products = get_products(cl), perror("get products"); return NULL)
            if (clq->is_enqueued)
                printf("%ld: NON VA BENEEEEEEEEEE---------------------------------------Servito: %d\n", cl->id, clq->served);
            //Reset del cliente in coda
            clq->served = 0;
            clq->processing = 0;
            DEBUG("[Thread Cliente %ld] Cliente %d: voglio acquistare %d prodotti\n", cl->id, client_id, clq->products)
            NOTZERO(get_store_state(&st_state), perror("get store state"); return NULL)
            if (clq->products > 0 && st_state != closed_fast_state) {
                do {
                    change_queue = 0;
                    //Entro nella migliore coda
                    do {
                        EQNULL(cassiere = get_best_queue(cl->casse, cl->k, NULL, -1), perror("get best queue"); return NULL)
                        MINUS1(err = join_queue(cassiere, clq, &queue_entrance), perror("join queue"); return NULL)
                        NOTZERO(get_store_state(&st_state), perror("get store state"); return NULL)
                    } while (st_state != closed_fast_state && !err);
                    DEBUG("[Thread Cliente %ld] Cliente %d: entro nella cassa %ld\n", cl->id, client_id, cassiere->id)
                    PTH(err, pthread_mutex_lock(&(cassiere)->mutex), perror("lock"); return NULL)
                    //Aspetto di essere servito oppure valuto se cambiare cassa
                    while (st_state != closed_fast_state && cassiere->isopen && !clq->served) {
                        if (clq->processing)    //Se il cassiere sta processando i miei prodotti allora aspetto che mi svegli
                            err = pthread_cond_wait(&(clq->waiting), &(cassiere->mutex));
                        else    //altrimenti faccio la timedwait ed eventualmente algoritmo di cambio cassa
                            err = pthread_cond_timedwait(&(clq->waiting), &(cassiere->mutex), &waitingtime);
                        if (err == -1 && errno != ETIMEDOUT) {
                            perror("timed wait");
                            return NULL;
                        }
                        if (errno == ETIMEDOUT) {
                            DEBUG("%ld: Farò l'algoritmo di cambio cassa primo o poi\n", cl->id)
                        }
                        PTH(err, pthread_mutex_unlock(&(cassiere)->mutex), perror("unlock"); return NULL)
                        NOTZERO(get_store_state(&st_state), perror("get store state"); return NULL)
                        PTH(err, pthread_mutex_lock(&(cassiere)->mutex), perror("lock"); return NULL)
                    }
                    if (clq->served) {
                        MINUS1(time_in_queue = elapsed_time(&queue_entrance),perror("elapsed ms from"); return NULL)
                        DEBUG("[Thread Cliente %ld] Cliente %d: Sono stato servito. Sono in coda: %d\n", cl->id, client_id, clq->is_enqueued)
                    } else if (st_state != closed_fast_state && !cassiere->isopen && !clq->served && !clq->processing) {
                        //Cassa chiusa e non sono stato servito ed il cassiere non mi sta servendo
                        DEBUG("%ld: CASSA CHIUSAAAAAAAAAAAAAAAA ----------------- LA CAMBIOOOOOOOOOOOOOOOO. Sono in coda: %d\n",
                              cl->id, clq->is_enqueued)
                        if (clq->is_enqueued) {
                            dequeue(cassiere, clq);
                        }
                        change_queue = 1;
                        queue_counter++;
                    } else if (st_state == closed_fast_state && clq->is_enqueued) {
                        dequeue(cassiere, clq);
                    }
                    PTH(err, pthread_mutex_unlock(&(cassiere)->mutex), perror("unlock"); return NULL)
                } while(change_queue);
                /*
                //Se sono ancora in coda ma non sono ancora stato servito
                if (!clq->processing && !clq->served) {
                    MINUS1(cost = get_queue_cost(cassiere, clq), perror("get queue cost"); return NULL)
                    PTH(err, pthread_mutex_unlock(&(cassiere)->mutex), perror("unlock"); return NULL)
                    //Valuto se le altre code sono migliori
                    EQNULL(best_cassiere = get_best_queue(cl->casse, cl->k, cassiere, cost, clq), perror("get best queue"); return NULL)
                    PTH(err, pthread_mutex_lock(&(cassiere)->mutex), perror("lock"); return NULL)
                    if (!clq->processing && !clq->served && best_cassiere != cassiere) {
                        dequeue(cassiere, clq)
                        PTH(err, pthread_mutex_unlock(&(cassiere)->mutex), perror("unlock"); return NULL)
                        err = join_queue(best_cassiere, clq, &queue_entrance);
                        if (err == -1 && errno != EAGAIN) {
                            perror("join_queue");
                            return NULL;
                        } else if (err == -1 && errno == EAGAIN) {
                            EQNULL(cassiere = join_best_queue(cl, clq), perror("join best queue"); return NULL)
                        } else {
                            cassiere = best_cassiere;
                        }
                        PTH(err, pthread_mutex_lock(&(cassiere)->mutex), perror("lock"); return NULL)
                    }
                }*/

                /*clstats->queue_counter++;
                do {
                    EQNULL(cassiere = get_best_queue(cl->casse, cl->k, cassiere, clq),perror("enter best queue"); return NULL)
                    errno = 0;
                    MINUS1(join_queue(cassiere, clq, &queue_entrance), perror("join queue"); return NULL)
                    NOTZERO(get_store_state(&st_state), perror("get store state"); return NULL)
                } while (st_state != closed_fast_state && errno == EAGAIN);
                DEBUG("[Thread Cliente %ld] Cliente %d: entro nella cassa %ld\n", cl->id, client_id, cassiere->id)

                PTH(err, pthread_mutex_lock(clq->mutex), perror("lock"); return NULL)
                while(st_state != closed_fast_state && !clq->served) {
                    PTH(err, pthread_cond_wait(&(clq->waiting), clq->mutex), perror("cond wait"); return NULL)
                    PTH(err, pthread_mutex_unlock(clq->mutex), perror("unlock"); return NULL)
                    NOTZERO(get_store_state(&st_state), perror("get store state"); return NULL)
                    PTH(err, pthread_mutex_lock(clq->mutex), perror("lock"); return NULL)
                }
                if (clq->served) {
                    clstats->products = clq->products;
                    MINUS1(clstats->time_in_queue = elapsed_time(&queue_entrance), perror("elapsed ms from"); return NULL)
                    DEBUG("[Thread Cliente %ld] Cliente %d: Sono stato servito\n", cl->id, client_id)
                }
                PTH(err, pthread_mutex_unlock(clq->mutex), perror("unlock"); return NULL)*/
            } else if (clq->products == 0 && ISOPEN(st_state)) {    //Chiedo permesso di uscita al direttore
                time_in_queue = 0;
                DEBUG("[Thread Cliente %ld] Cliente %d: Chiedo permesso di uscita\n", cl->id, client_id)
                MINUS1(ask_exit_permission(cl->id), perror("ask exit permission"); return NULL)
                MINUS1(wait_permission(cl), perror("wait permission"); return NULL)
            }
            MINUS1(leave_store(cl->store), perror("exit store"); exit(EXIT_FAILURE))
            if (st_state != closed_fast_state) {    //Il cliente è stato sicuramente servito
                MINUS1(time_in_store = elapsed_time(&store_entrance), perror("elapsed ms from"); return NULL)
                MINUS1(log_client_stats(stats, client_id, clq->products, time_in_store, time_in_queue, queue_counter), 
                       perror("log client stats"); return NULL)
            }
            DEBUG("[Thread Cliente %ld] Cliente %d: sono uscito dal supermercato\n", cl->id, client_id)
        }
        NOTZERO(get_store_state(&st_state), perror("get store state"); return NULL)
    }
    DEBUG("[Thread Cliente %ld] termino\n", cl->id)
    if (clq->is_enqueued)
        printf("%ld: IN CODAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n", cl->id);
    MINUS1(destroy_client_in_queue(clq), perror("destroy client in queue"); return NULL)
    return stats;
}

static int get_products(client_t *cl) {
    unsigned int seed = cl->id;
    int products = RANDOM(seed, 0, cl->p);
    int time = RANDOM(seed, MIN_T, cl->t);
    MINUS1(msleep(time), return -1)
    return products;
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
    PTH(err, pthread_mutex_lock(&(cl->mutex)), return -1)
    while (cl->can_exit == 0) {
        PTH(err, pthread_cond_wait(&(cl->exit_permission), &(cl->mutex)), return -1)
    }
    cl->can_exit = 0;   //resetto per la prossima volta che chiederò di uscire
    PTH(err, pthread_mutex_unlock(&(cl->mutex)), return -1)
    return 0;
}