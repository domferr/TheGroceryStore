#define DEBUGGING 0
#include "../include/utils.h"
#include "../include/store.h"
#include "../include/client.h"
#include "../include/config.h"
#include "../include/log.h"
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

/**
 * Rimane il attesa che arrivi una risposta alla richiesta di uscita dal supermercato. Chiamata bloccante.
 *
 * @param cl struttura dati del cliente
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno
 */
static int wait_permission(client_t *cl);

void *client_thread_fun(void *args) {
    client_t *cl = (client_t *) args;
    int client_id, err, cost, change_queue, queue_counter, enqueued;
    long time_in_store, time_in_queue;
    struct timespec store_entrance, queue_entrance;
    store_state st_state;
    list_t *stats;
    client_in_queue_t *clq;
    cassiere_t *cassiere = NULL, *best_cassiere;

    EQNULLERR(stats = queue_create(), return NULL)
    EQNULLERR(clq = alloc_client_in_queue(), return NULL)

    MINUS1ERR(get_store_state(cl->store, &st_state), return NULL)
    while (ISOPEN(st_state)) {
        //Resetto i contatori delle statistiche
        queue_counter = 0;
        //Entro nel supermercato. Chiamata bloccante. Il controllo ritorna se entro nel supermercato o se esso viene chiuso
        client_id = enter_store(cl->store);
        if (client_id) {
            MINUS1ERR(clock_gettime(CLOCK_MONOTONIC, &store_entrance), return NULL)
            DEBUG("[Thread Cliente %ld] Cliente %d: sono entrato nel supermercato\n", cl->id, client_id)
            MINUS1ERR(clq->products = get_products(cl), return NULL)
            if (clq->is_enqueued)
                printf("%ld: NON VA BENEEEEEEEEEE---------------------------------------Servito: %d\n", cl->id, clq->served);
            //Reset del cliente in coda
            clq->served = 0;
            clq->processing = 0;
            DEBUG("[Thread Cliente %ld] Cliente %d: voglio acquistare %d prodotti\n", cl->id, client_id, clq->products)
            MINUS1ERR(get_store_state(cl->store, &st_state), return NULL)
            if (clq->products > 0 && st_state != closed_fast_state) {
                do {
                    change_queue = 0;
                    EQNULLERR(cassiere = get_best_queue(cl->casse, cl->k, NULL, -1), return NULL)
                    MINUS1ERR(get_store_state(cl->store, &st_state), return NULL)
                    if (st_state == closed_fast_state) break;
                    PTHERR(err, pthread_mutex_lock(&(cassiere)->mutex), return NULL)
                    MINUS1ERR(enqueued = join_queue(cassiere, clq, &queue_entrance), return NULL)
                    //Aspetto di essere servito oppure valuto se cambiare cassa. Se il cassiere sta processando i miei
                    //prodotti non esco mai perchè devo aspettare che il cassiere finisca
                    if (enqueued) {
                        while (clq->processing || (st_state != closed_fast_state && cassiere->isopen && !clq->served)) {
                            //Se il cassiere sta processando i miei prodotti allora aspetto che mi svegli
                            if (clq->processing) {
                                PTHERR(err, pthread_cond_wait(&(clq->waiting), &(cassiere->mutex)), return NULL)
                                PTHERR(err, pthread_mutex_unlock(&(cassiere)->mutex), return NULL)
                                MINUS1ERR(get_store_state(cl->store, &st_state), return NULL)
                                PTHERR(err, pthread_mutex_lock(&(cassiere)->mutex), return NULL)
                            } else {    //altrimenti attendo per un tot di secondi e faccio l'algoritmo di cambio cassa
                                PTHERR(err, pthread_mutex_unlock(&(cassiere)->mutex), return NULL)
                                MINUS1ERR(msleep(cl->s), return NULL)
                                MINUS1ERR(get_store_state(cl->store, &st_state), return NULL)
                                PTHERR(err, pthread_mutex_lock(&(cassiere)->mutex), return NULL)
                                if (st_state != closed_fast_state && !clq->processing && !clq->served && cassiere->isopen) {
                                    MINUS1ERR(cost = get_queue_cost(cassiere, clq), return NULL)
                                    PTHERR(err, pthread_mutex_unlock(&(cassiere)->mutex), return NULL)
                                    EQNULLERR(best_cassiere = get_best_queue(cl->casse, cl->k, cassiere, cost), return NULL)
                                    MINUS1ERR(get_store_state(cl->store, &st_state), return NULL)
                                    PTHERR(err, pthread_mutex_lock(&(cassiere)->mutex), return NULL)
                                    if (cassiere != best_cassiere && st_state != closed_fast_state
                                        && !clq->processing && !clq->served && cassiere->isopen) {
                                        leave_queue(cassiere, clq);
                                        PTHERR(err, pthread_mutex_unlock(&(cassiere)->mutex), return NULL)
                                        do {
                                            MINUS1ERR(get_store_state(cl->store, &st_state), return NULL)
                                            if (st_state == closed_fast_state) break;
                                            PTHERR(err, pthread_mutex_lock(&(best_cassiere)->mutex), return NULL)
                                            MINUS1ERR(enqueued = join_queue(best_cassiere, clq, &queue_entrance), return NULL)
                                            if (!enqueued) {
                                                PTHERR(err, pthread_mutex_unlock(&(best_cassiere)->mutex), return NULL)
                                                EQNULLERR(best_cassiere = get_best_queue(cl->casse, cl->k, NULL, -1), return NULL)
                                            } else {
                                                //printf("[%d] Cambio cassa da %ld a %ld\n", client_id, cassiere->id, best_cassiere->id);
                                                cassiere = best_cassiere;
                                                queue_counter++;
                                            }
                                        } while (!enqueued && st_state != closed_fast_state);
                                    }
                                }
                            }
                        }
                    }
                    if (clq->served) {
                        MINUS1ERR(time_in_queue = elapsed_time(&queue_entrance), return NULL)
                        DEBUG("[Thread Cliente %ld] Cliente %d: Sono stato servito. Sono in coda: %d\n", cl->id, client_id, clq->is_enqueued)
                    } else if (st_state != closed_fast_state && !cassiere->isopen && !clq->served && !clq->processing) {
                        //Cassa chiusa e non sono stato servito ed il cassiere non mi sta servendo
                        leave_queue(cassiere, clq);
                        change_queue = 1;
                        queue_counter++;
                    } else if (st_state == closed_fast_state) {
                        leave_queue(cassiere, clq);
                    }
                    PTHERR(err, pthread_mutex_unlock(&(cassiere)->mutex), return NULL)
                } while (st_state != closed_fast_state && change_queue);
            } else if (clq->products == 0 && ISOPEN(st_state)) {    //Chiedo permesso di uscita al direttore
                time_in_queue = 0;
                DEBUG("[Thread Cliente %ld] Cliente %d: Chiedo permesso di uscita\n", cl->id, client_id)
                MINUS1ERR(ask_exit_permission(cl->id), return NULL)
                MINUS1ERR(wait_permission(cl), return NULL)
            }
            MINUS1ERR(leave_store(cl->store), return NULL)
            if (clq->products == 0 || clq->served) {
                MINUS1ERR(time_in_store = elapsed_time(&store_entrance), return NULL)
                MINUS1ERR(log_client_stats(stats, client_id, clq->products, time_in_store, time_in_queue, queue_counter), return NULL)
            }
            DEBUG("[Thread Cliente %ld] Cliente %d: sono uscito dal supermercato\n", cl->id, client_id)
        }
        MINUS1ERR(get_store_state(cl->store, &st_state), return NULL)
    }
    DEBUG("[Thread Cliente %ld] termino\n", cl->id)
    if (clq->is_enqueued)
        printf("%ld: IN CODAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n", cl->id);
    MINUS1ERR(destroy_client_in_queue(clq), return NULL)
    return stats;
}

static int get_products(client_t *cl) {
    unsigned int seed = cl->id + time(NULL);
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

static int wait_permission(client_t *cl) {
    int err;
    PTH(err, pthread_mutex_lock(&(cl->mutex)), return -1)
    while (cl->can_exit == 0) {
        PTH(err, pthread_cond_wait(&(cl->exit_permission), &(cl->mutex)), return -1)
    }
    cl->can_exit = 0;   //resetto per la prossima volta che chiederò di uscire
    PTH(err, pthread_mutex_unlock(&(cl->mutex)), return -1)
    return 0;
}