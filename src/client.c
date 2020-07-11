#define _POSIX_C_SOURCE 200809L
#include "client.h"
#include "grocerystore.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

//#define DEBUGCLIENT
#define MSMULTIPLIER 1000000

void *client_fun(void *args) {
    int run = 1;
    struct timespec ts = {0};
    gs_state state;
    client_t *cl = (client_t*) args;
    grocerystore_t *gs = (grocerystore_t*) cl->gs;
#ifdef DEBUGCLIENT
    printf("Cliente %ld attivo\n", cl->id);
#endif
    while (run) {
#ifdef DEBUGCLIENT
        printf("Cliente %ld: Aspetto all'ingresso del supermercato\n", cl->id);
#endif
        //Entro nel supermercato. Chiamata bloccante, perchè attende che si possa entrare o che il supermercato chiuda
        state = enter_store(gs, cl->id);    //Stato in cui sono fermo all'ingresso
        if (ISOPEN(state)) {    //Stato in cui sono dentro al supermercato
#ifdef DEBUGCLIENT
            printf("Cliente %ld: Entro nel supermercato\n", cl->id);
#endif
            run = 1;
            ts.tv_nsec = 1000*MSMULTIPLIER;
            if (nanosleep(&ts, NULL) != 0)
                perror("nanosleep");
            //Esco dal supermercato
            exit_store(gs);
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

client_t *alloc_client(size_t id, grocerystore_t *gs) {
    client_t *client = (client_t*) malloc(sizeof(client_t));
    if (client == NULL)
        return NULL;
    client->gs = gs;
    client->id = id;
    return client;
}