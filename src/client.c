#define _POSIX_C_SOURCE 200809L
#include "client.h"
#include "grocerystore.h"
#include "config.h"
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

//#define DEBUGCLIENT

#define MS_TO_SEC(ms) \
    ms/1000
#define MS_TO_NANOSEC(ms) \
    ms%1000000
#define RANDOM(seed, min, max) \
    (rand_r(&seed)%(max-min)) + min

static int msleep(int milliseconds);

void *client_fun(void *args) {
    int run = 1;
    gs_state state;
    client_t *cl = (client_t*) args;
    grocerystore_t *gs = (grocerystore_t*) cl->gs;
    unsigned int seed = cl->id;
    int random_time;
    int products;
#ifdef DEBUGCLIENT
    printf("Thread cliente %ld in esecuzione\n", cl->id);
#endif
    while (run) {
        //Entro nel supermercato. Chiamata bloccante, perchè attende che si possa entrare o che il supermercato chiuda
        state = enter_store(gs, cl->id);
        if (ISOPEN(state)) {    //Stato in cui sono dentro al supermercato
            products = RANDOM(seed, 0, cl->p);
            random_time = RANDOM(seed, MIN_T, cl->t);
#ifdef DEBUGCLIENT
            printf("Cliente %ld: Entro nel supermercato. Prendo %d prodotti in %dms\n", cl->id, products, random_time);
#endif
            if (msleep(random_time) != 0) {
                perror("client sleep");
                return NULL;
            }
            //Esco dal supermercato
            exit_store(gs);
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
    if (client == NULL)
        return NULL;
    client->gs = gs;
    client->id = id;
    client->t = t;
    client->p = p;
    return client;
}

static int msleep(int milliseconds) {
    struct timespec req = {MS_TO_SEC(milliseconds), MS_TO_NANOSEC(milliseconds)}, rem = {0, 0};
    int res = EINTR;

    while (res == EINTR) {
        rem.tv_sec = 0;
        rem.tv_nsec = 0;
        res = nanosleep(&req, &rem);
        req = rem;
    }

    return res;
}