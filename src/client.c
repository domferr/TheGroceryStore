#define _POSIX_C_SOURCE 199309L
#include "client.h"
#include "grocerystore.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define MSMULTIPLIER 1000000

int msleep(int milliseconds);

void *client_fun(void *args) {
    int run = 1;
    gs_state state;
    client_t *cl = (client_t*) args;
    grocerystore_t *gs = (grocerystore_t*) cl->gs;
    printf("Cliente %ld attivo\n", cl->id);
    while (run) {
        printf("Cliente %ld: Aspetto all'ingresso del supermercato\n", cl->id);
        //Entro nel supermercato. Chiamata bloccante, perchè attende che si possa entrare o che il supermercato chiuda
        state = enter_store(gs, cl->id);    //Stato in cui sono fermo all'ingresso
        if (ISOPEN(state)) {    //Stato in cui sono dentro al supermercato
            printf("Cliente %ld: Entro nel supermercato\n", cl->id);
            run = 1;
            //Esco dal supermercato
            exit_store(gs);
        } else {    //Sono stato svegliato perchè devo terminare. Situazione di chiusura
            run = 0;
        }
    }
    printf("Cliente %ld terminato\n", cl->id);
    free(cl);
    return 0;
}

int msleep(int milliseconds) {
    struct timespec ts_sleep = {0, milliseconds*MSMULTIPLIER}, remainder;

    return nanosleep(&ts_sleep, &remainder);
}