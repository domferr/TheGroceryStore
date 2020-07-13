#include "client.h"
#include "grocerystore.h"
#include "config.h"
#include "utils.h"
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

//#define DEBUGCLIENT

void *client_fun(void *args) {
    client_t *cl = (client_t*) args;
    grocerystore_t *gs = (grocerystore_t*) cl->gs;
    unsigned int seed = cl->id;
    int run = 1;
    int random_time;
    int products;
    int is_inside_store;
#ifdef DEBUGCLIENT
    printf("Thread cliente %ld in esecuzione\n", cl->id);
#endif
    while (run) {
        //Entro nel supermercato. Chiamata bloccante, perchè attende che si possa entrare o che il supermercato chiuda
        is_inside_store = enter_store(gs);
        MINUS1(is_inside_store, perror("enter store"); return NULL)
        if (is_inside_store) {    //Stato in cui sono dentro al supermercato
            products = RANDOM(seed, 0, cl->p);
            random_time = RANDOM(seed, MIN_T, cl->t);
#ifdef DEBUGCLIENT
            printf("Cliente %ld: Entro nel supermercato. Prendo %d prodotti in %dms\n", cl->id, products, random_time);
#endif
            NOTZERO(msleep(random_time), perror("client sleep"); free(cl); return NULL)
            //Esco dal supermercato
            NOTZERO(exit_store(gs), perror("exit store"); return NULL)
#ifdef DEBUGCLIENT
            printf("Cliente %ld: Esco dal supermercato\n", cl->id);
#endif
        } else {    //Sono stato svegliato perchè devo terminare. Situazione di chiusura
            run = 0;
        }
    }
#ifdef DEBUGCLIENT
    printf("Cliente %ld terminato\n", cl->id);
#endif
    free(cl);
    return 0;
}

client_t *alloc_client(size_t id, grocerystore_t *gs, int t, int p) {
    client_t *client = (client_t*) malloc(sizeof(client_t));
    EQNULL(client, return NULL;)
    client->gs = gs;
    client->id = id;
    client->t = t;
    client->p = p;
    return client;
}