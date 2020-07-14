#include "client.h"
#include "client_in_queue.h"
#include "cashier.h"
#include "grocerystore.h"
#include "config.h"
#include "utils.h"
#include "logger.h"
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define DEBUGCLIENT

void *client_fun(void *args) {
    client_t *cl = (client_t*) args;
    grocerystore_t *gs = (grocerystore_t*) cl->gs;
    int err, products, is_inside_store;
    cashier_t *picked_cashier;
    gs_state store_state;

    log_client *log = alloc_client_log(cl->id);
    EQNULL(log, perror("alloc client log"); return NULL);

    client_in_queue *cl_in_q = alloc_client_in_queue(cl->id);
    EQNULL(cl_in_q, perror("alloc client in queue"); return NULL);

#ifdef DEBUGCLIENT
    printf("Thread cliente %ld in esecuzione\n", cl->id);
#endif

    NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
    while (ISOPEN(store_state)) {
        //Entro nel supermercato. Chiamata bloccante, perchè attende che si possa entrare o che il supermercato chiuda
        is_inside_store = enter_store(gs, &store_state);
        MINUS1(is_inside_store, perror("enter store"); return NULL)
        //Se sono stato fatto entrare allora lo store è aperto, altrimenti lo store è chiuso e termino
        if (is_inside_store) {    //Stato in cui sono dentro al supermercato
            products = client_get_products(cl, gs, &store_state);
            MINUS1(products, perror("client get products"); return NULL)
            //Il cliente deve cercare una cassa se e solo se ha prodotti > 0 e lo stato del supermercato è open oppure closed
            if (store_state != closed_fast && products != 0) {
                picked_cashier = NULL;
                cl_in_q->products = products;
                cl_in_q->status = waiting;
                //Entro in una coda di una cassa aperta random
                picked_cashier = enter_random_queue(cl, cl_in_q);
                EQNULL(picked_cashier, perror("client enter random queue"); return NULL)
                PTH(err, pthread_mutex_lock(&(cl_in_q)->mutex), perror("client mutex lock"); return NULL)
                //Fino a quando non sono stato servito oppure lo store è aperto o chiuso tramite SIGHUP
                while (cl_in_q->status == waiting) {
                    PTH(err, pthread_cond_wait(&(cl_in_q->waiting), (&(cl_in_q->mutex))), perror("client cond wait"); return NULL)
                    if (cl_in_q->status == cashier_sleeping) {
                        cl_in_q->status = waiting;
                        PTH(err, pthread_mutex_unlock(&(cl_in_q)->mutex), perror("client mutex unlock"); return NULL)
                        //Entro in una coda di una cassa aperta random
                        picked_cashier = enter_random_queue(cl, cl_in_q);
                        EQNULL(picked_cashier, perror("client enter random queue"); return NULL)
                        PTH(err, pthread_mutex_lock(&(cl_in_q)->mutex), perror("client mutex lock"); return NULL)
                    }
                }
                PTH(err, pthread_mutex_unlock(&(cl_in_q->mutex)), return NULL)
            } else if (store_state != closed_fast) {
                //TODO Aspetto il permesso del direttore per uscire
            }
            //Esco dal supermercato
            NOTZERO(exit_store(gs), perror("exit store"); return NULL)
#ifdef DEBUGCLIENT
            printf("Cliente %ld: Esco dal supermercato\n", cl->id);
#endif
        }
    }
#ifdef DEBUGCLIENT
    printf("Cliente %ld terminato\n", cl->id);
#endif

    MINUS1(destroy_client_in_queue(cl_in_q), perror("destroy client in queue"); return NULL)
    free(cl);
    return log;
}

client_t *alloc_client(size_t id, grocerystore_t *gs, int t, int p, cashier_t **cashiers, size_t no_of_cashiers) {
    client_t *client = (client_t*) malloc(sizeof(client_t));
    EQNULL(client, return NULL;)
    client->gs = gs;
    client->id = id;
    client->cashiers = cashiers;
    client->no_of_cashiers = no_of_cashiers;
    client->t = t;
    client->p = p;
    return client;
}

int client_get_products(client_t *cl, grocerystore_t *gs, gs_state *store_state) {
    unsigned int seed = cl->id;
    int products = RANDOM(seed, 0, cl->p);
    int random_time = RANDOM(seed, MIN_T, cl->t);
#ifdef DEBUGCLIENT
    printf("Cliente %ld: Entro nel supermercato. Prendo %d prodotti in %dms\n", cl->id, products, random_time);
#endif
    NOTZERO(msleep(random_time), return -1)
    NOTZERO(get_store_state(gs, store_state), perror("get store state"); return -1)
    return products;
}

cashier_t *enter_random_queue(client_t *cl, client_in_queue *cl_in_q) {
    int found = 0, cashier_index, err;
    unsigned int seed = cl->id;
    cashier_t **cashiers = cl->cashiers;
    cashier_t *picked_cashier;  //Cassiere scelto casualmente

    while(!found) {
        //0 <= cashier_index < no_of_cashiers
        cashier_index = RANDOM(seed, 0, cl->no_of_cashiers);
        picked_cashier = cashiers[cashier_index];

        PTH(err, pthread_mutex_lock(&(picked_cashier->mutex)), return NULL)
        //Se la cassa scelta casualmente è attiva, mi aggiungo in coda ed esco dal ciclo
        if (picked_cashier->state == active) {
            MINUS1(push(picked_cashier->queue, cl_in_q), return NULL);
            found = 1;
            if ((picked_cashier->queue)->size == 1) {
#ifdef DEBUGCLIENT
                printf("Cliente %ld: segnalo il cassiere %ld\n", cl->id, picked_cashier->id);
#endif
                PTH(err, pthread_cond_signal(&(picked_cashier->noclients)), return NULL)
            }
        }
        PTH(err, pthread_mutex_unlock(&(picked_cashier->mutex)), return NULL)
    }
#ifdef DEBUGCLIENT
    printf("Cliente %ld: mi metto in coda alla cassa numero %ld\n", cl->id, picked_cashier->id);
#endif
    return picked_cashier;
}