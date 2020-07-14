#include "client.h"
#include "cashier.h"
#include "grocerystore.h"
#include "config.h"
#include "utils.h"
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define DEBUGCLIENT

void *client_fun(void *args) {
    int products, is_inside_store, err, client_served;
    client_t *cl = (client_t*) args;
    grocerystore_t *gs = (grocerystore_t*) cl->gs;
    gs_state store_state;
    cashier_state ca_state;
    cashier_t *picked_cashier;
#ifdef DEBUGCLIENT
    printf("Thread cliente %ld in esecuzione\n", cl->id);
#endif

    client_in_queue *cl_in_q;/* = (client_in_queue*) malloc(sizeof(client_in_queue));
    EQNULL(cl_in_q, perror("malloc client in queue"); return NULL)
    PTH(err, pthread_mutex_init(&(cl_in_q->mutex), NULL), perror("client mutex init"); return NULL)
    PTH(err, pthread_cond_init(&(cl_in_q->waiting), NULL), perror("client cond init"); return NULL)*/

    NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
    while(ISOPEN(store_state)) {
        //Entro nel supermercato. Chiamata bloccante, perchè attende che si possa entrare o che il supermercato chiuda
        is_inside_store = enter_store(gs, &store_state);
        MINUS1(is_inside_store, perror("enter store"); return NULL)
        if (is_inside_store) {    //Stato in cui sono dentro al supermercato
            //products = client_get_products(cl);
            //MINUS1(products, perror("client get products"); return NULL);
            //NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
            //Il cliente deve cercare una cassa se e solo se ha prodotti > 0 e lo stato del supermercato è open oppure closed
            /*products = 0;
            if (store_state != closed_fast && products == 0){
                //TODO Chiedo al direttore se posso uscire
            } else if (store_state != closed_fast && products != 0) {
                cl_in_q->done = 0;
                cl_in_q->products = products;
                client_served = 0;
                //Fino a quando non sono stato servito è il supermercato è aperto oppure chiuso con SIGHUP
                while (store_state != closed_fast && !client_served) {
                    //Entro in una coda di una cassa aperta random
                    picked_cashier = enter_random_queue(cl, cl_in_q);
                    EQNULL(picked_cashier, perror("client enter random queue"); return NULL)
                    client_served = client_wait_in_queue(cl_in_q, gs, picked_cashier, &store_state, &ca_state);
                    MINUS1(client_served, perror("client wait in queue"); return NULL)
                }
            }*/
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
    //free(cl_in_q);
    free(cl);
    return 0;
}

client_t *alloc_client(size_t id, grocerystore_t *gs, size_t no_of_cashiers, cashier_t **cashiers, int t, int p) {
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

int client_get_products(client_t *cl) {
    unsigned int seed = cl->id;
    int products = RANDOM(seed, 0, cl->p);
    int random_time = RANDOM(seed, MIN_T, cl->t);
#ifdef DEBUGCLIENT
    printf("Cliente %ld: Entro nel supermercato. Prendo %d prodotti in %dms\n", cl->id, products, random_time);
#endif
    NOTZERO(msleep(random_time), return -1)
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
        }
        PTH(err, pthread_mutex_unlock(&(picked_cashier->mutex)), return NULL)
    }
#ifdef DEBUGCLIENT
    printf("Cliente %ld: mi metto in coda alla cassa numero %ld\n", cl->id, picked_cashier->id);
#endif
    return picked_cashier;
}