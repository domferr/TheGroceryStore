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

typedef enum {
    on_entrance,    //Stato in cui si attende di poter entrare nel supermercato o che quest'ultimo chiuda
    take_products,  //Stato in cui si simula di prendere un certo numero di prodotti in un certo tempo
    enter_queue,    //Stato in cui ci si accoda in una cassa per pagare
    on_exit         //Stato in cui il cliente esce dal supermercato. Vi rientrerà solo se il supermercato è ancora aperto
} cl_state;

void *client_fun(void *args) {
    client_t *cl = (client_t*) args;
    int err, random_time, client_id, run = 1;
    int store_entrance_timestamp = 0, store_exit_timestamp = 0, queue_entrance_timestamp = 0, queue_exit_timestamp = 0;
    unsigned int seed = cl->id;
    cl_state internal_state = on_entrance;
    gs_state store_state;
    client_stats *current_client_stats = alloc_client_stats(cl->id);
    grocerystore_t *gs = (grocerystore_t*) cl->gs;
    client_thread_stats *stats = alloc_client_thread_stats(cl->id);
    EQNULL(stats, perror("alloc client thread stats"); return NULL)
    client_in_queue *cl_in_q = alloc_client_in_queue(cl->id);
    EQNULL(cl_in_q, perror("alloc client in queue"); return NULL)
    NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)

    while(run) {
        switch(internal_state) {
            case on_entrance:
                client_id = enter_store(gs, &store_state);
                MINUS1(client_id, perror("Impossibile entrare nello store"); return NULL)
                if (client_id) {    //Se sono entrato nel supermercato
                    internal_state = take_products;
                    run = 1;
                } else {    //Devo terminare perchè ero all'ingresso ed il supermercato sta chiudendo
                    run = 0;
                }
                break;
            case take_products:
                cl_in_q->products = RANDOM(seed, 0, cl->p);
                random_time = RANDOM(seed, MIN_T, cl->t);
#ifdef DEBUGCLIENT
                printf("Cliente %ld: Entro nel supermercato. Prendo %d prodotti in %dms\n", cl->id, cl_in_q->products, random_time);
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
                current_client_stats->queue_counter += 1;
                //TODO set queue_entrance_timestamp

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
                if (cl_in_q->status == done || store_state == closed_fast) {
                    //TODO set tempo di uscita store_exit_timestamp
                    current_client_stats->time_in_store = store_exit_timestamp - store_entrance_timestamp;
                    internal_state = on_exit;
                }
                break;
            case on_exit:
                if (cl_in_q->products == 0) {
                    //TODO Aspetto il permesso del direttore e poi esco dal supermercato
                }
#ifdef DEBUGCLIENT
                printf("Cliente %ld: esco dal supermercato\n", cl->id);
#endif
                NOTZERO(exit_store(gs, &store_state), perror("exit store"); return NULL)
                //Rendo il thread riutilizzabile se il supermercato è aperto
                internal_state = on_entrance;
                run = store_state != closed_fast;
                break;
        }
    }
#ifdef DEBUGCLIENT
    printf("Cliente %ld terminato\n", cl->id);
#endif

    MINUS1(destroy_client_in_queue(cl_in_q), perror("destroy client in queue"); return NULL)
    free(current_client_stats);
    free(cl);
    return stats;
}

/*void *client_fun(void *args) {
    int err, client_id, entrance_timestamp = 0, exit_timestamp = 0, queue_entrance_timestamp = 0, queue_exit_timestamp = 0;
    client_stats *current_client_stats;

    client_t *cl = (client_t*) args;
    grocerystore_t *gs = (grocerystore_t*) cl->gs;

    cashier_t *picked_cashier;
    gs_state store_state;

    client_thread_stats *stats = alloc_client_thread_stats(cl->id);
    EQNULL(stats, perror("alloc client thread stats"); return NULL)
    client_in_queue *cl_in_q = alloc_client_in_queue(cl->id);
    EQNULL(cl_in_q, perror("alloc client in queue"); return NULL);

#ifdef DEBUGCLIENT
    printf("Thread cliente %ld in esecuzione\n", cl->id);
#endif

    NOTZERO(get_store_state(gs, &store_state), perror("get store state"); return NULL)
    while (ISOPEN(store_state)) {
        //Entro nel supermercato. Chiamata bloccante, perchè attende che si possa entrare o che il supermercato chiuda
        client_id = enter_store(gs, &store_state);
        MINUS1(client_id, perror("enter store"); return NULL)
        //Se sono stato fatto entrare allora lo store è aperto, altrimenti lo store è chiuso e termino
        if (client_id) {    //Stato in cui sono dentro al supermercato
            current_client_stats = alloc_client_stats(client_id);
            push(stats->queue, current_client_stats);
            //TODO set tempo di entrata entrance_timestamp
            cl_in_q->products = client_get_products(cl, gs, &store_state);
            MINUS1(cl_in_q->products, perror("client get products"); return NULL)
            //Il cliente deve cercare una cassa se e solo se ha prodotti > 0 e lo stato del supermercato è open oppure closed
            if (store_state != closed_fast && cl_in_q->products != 0) {
                cl_in_q->status = waiting;
                //Entro in una coda di una cassa aperta random
                err = enter_random_queue(&picked_cashier, cl, cl_in_q, gs, &store_state);
                current_client_stats->queue_counter += 1;
                //TODO set queue_entrance_timestamp
                MINUS1(err, perror("client enter random queue"); return NULL)
                PTH(err, pthread_mutex_lock(&(cl_in_q)->mutex), perror("client mutex lock"); return NULL)
                //Fino a quando non sono stato servito oppure lo store è aperto o chiuso tramite SIGHUP
                while (cl_in_q->status == waiting) {
                    PTH(err, pthread_cond_wait(&(cl_in_q->waiting), (&(cl_in_q->mutex))), perror("client cond wait"); return NULL)
                    if (cl_in_q->status == cashier_sleeping) {
                        //Cambio cassa perchè la cassa è chiusa!
                        cl_in_q->status = waiting;
                        current_client_stats->queue_counter += 1;
                        //TODO set queue_entrance_timestamp
                        PTH(err, pthread_mutex_unlock(&(cl_in_q)->mutex), perror("client mutex unlock"); return NULL)
                        //Entro in una coda di una cassa aperta random
                        err = enter_random_queue(&picked_cashier, cl, cl_in_q, gs, &store_state);
                        MINUS1(err, perror("client enter random queue"); return NULL)
                        PTH(err, pthread_mutex_lock(&(cl_in_q)->mutex), perror("client mutex lock"); return NULL)
                    } else if (cl_in_q->status == done) {
                        current_client_stats->products = cl_in_q->products;
                        //TODO set queue_exit_timestamp
                        current_client_stats->time_in_queue = queue_exit_timestamp - queue_entrance_timestamp;
                    }
                }
                PTH(err, pthread_mutex_unlock(&(cl_in_q->mutex)), return NULL)
            } else if (store_state != closed_fast) {
                //TODO Aspetto il permesso del direttore per uscire
            }
            //Esco dal supermercato
            NOTZERO(exit_store(gs), perror("exit store"); return NULL)
            //TODO set tempo di uscita exit_timestamp
            current_client_stats->time_in_store = exit_timestamp - entrance_timestamp;

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
    return stats;
}*/

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

int enter_random_queue(client_t *cl, client_in_queue *cl_in_q, grocerystore_t *gs, gs_state *store_state) {
    int found = 0, cashier_index, err;
    unsigned int seed = cl->id;
    cashier_t **cashiers = cl->cashiers;
    cashier_t *random_cashier;

    while(ISOPEN(*store_state) && !found) {
        //0 <= cashier_index < no_of_cashiers
        cashier_index = RANDOM(seed, 0, cl->no_of_cashiers);
        random_cashier = cashiers[cashier_index];

        PTH(err, pthread_mutex_lock(&(random_cashier->mutex)), return -1)
        //Se la cassa scelta casualmente è attiva, mi aggiungo in coda ed esco dal ciclo
        if (random_cashier->state == active) {
            MINUS1(push(random_cashier->queue, cl_in_q), return -1);
            if ((random_cashier->queue)->size == 1)
                PTH(err, pthread_cond_signal(&(random_cashier->noclients)), return -1)
            found = 1;
        }
        PTH(err, pthread_mutex_unlock(&(random_cashier->mutex)), return -1)
        NOTZERO(get_store_state(gs, store_state), return -1)
    }
#ifdef DEBUGCLIENT
    if (found) {
        printf("Cliente %ld: mi metto in coda alla cassa numero %ld\n", cl->id, random_cashier->id);
    }
#endif
    return 0;
}