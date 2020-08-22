#define DEBUGGING 0
#include "../libs/utils/include/utils.h"
#include "../include/store.h"
#include "../include/client.h"
#include "../include/config.h"
#include "../include/log.h"
#include "../include/client_in_queue.h"
#include "../include/cassa_queue.h"
#include <stdlib.h>
#include <stdio.h>



/**
 * Rimane il attesa che arrivi una risposta alla richiesta di uscita dal supermercato. Chiamata bloccante.
 *
 * @param cl struttura dati del cliente
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno
 */
static int wait_permission(client_t *cl);

/**
 * Prende un certo numero di prodotti in un certo periodo il tempo. I prodotti sono maggiori o uguali a zero e minori
 * del parametro p. Il tempo necessario è maggiore o uguale a MIN_T e minore del parametro t.
 *
 * @param cl struttura dati del cliente
 * @return numero di prodotti in caso di successo, -1 altrimenti ed imposta errno
 */
static int get_products(client_t *cl);

/**
 * Aspetta nella coda del cassiere passato per argomento. Chiamata bloccante, e viene restituito controllo al chiamante
 * se avviene una delle seguenti:
 * - il supermercato è in chiusura veloce ed il cliente non è processato dal cassiere quindi deve uscire dal supermercato
 * - bisogna cambiare cassa perchè la cassa è stata chiusa ed il supermercato è ancora aperto o è chiuso normalmente
 * - bisogna cambiare cassa perchè c'è una cassa migliore a cui accodarsi
 *
 * Quando bisogna cambiare cassa, 1 viene ritornato ed il puntatore better_cass viene aggiornato con la migliore cassa
 * a cui accodarsi. Se non bisogna cambiare cassa, 0 viene ritornato ed il puntatore better_cass non viene aggiornato.
 * In caso di errore, ritorna -1 ed imposta errno.
 *
 * @param cl struttura dati del cliente
 * @param clq struttura dati del cliente in coda
 * @param cassiere struttura dati del cassiere in cui si è in coda
 * @param better_cass puntatore alla struttura dati da aggiornare con la migliore cassa in caso sia necessario cambiare cassa
 * @param st_state variabile da aggiornare con lo stato del supermercato
 * @return 1 se bisogna cambiare cassa ed imposta better_cass oppure 0 in caso contrario e non imposta better_cass.
 * Ritorna -1 in caso di errore ed imposta errno.
 */
static int wait_in_queue(client_t *cl, client_in_queue_t *clq, cassiere_t *cassiere, cassiere_t *better_cass, store_state *st_state);

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

void *client_thread_fun(void *args) {
    client_t *cl = (client_t *) args;
    int client_id, err, change_queue, queue_counter, enqueued;
    long time_in_store, time_in_queue;
    struct timespec store_entrance, queue_entrance;
    store_state st_state;
    list_t *stats;
    client_in_queue_t *clq;
    cassiere_t *cassiere = NULL, *better_cass = NULL;

    EQNULLERR(stats = list_create(), return NULL)
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
            //Reset del cliente in coda
            clq->served = 0;
            clq->processing = 0;
            DEBUG("[Thread Cliente %ld] Cliente %d: voglio acquistare %d prodotti\n", cl->id, client_id, clq->products)
            MINUS1ERR(get_store_state(cl->store, &st_state), return NULL)
            if (clq->products > 0 && st_state != closed_fast_state) {
                change_queue = 0;
                better_cass = NULL;
                do {
                    if (better_cass == NULL) {  //Se non ho il puntatore alla cassa migliore
                        EQNULLERR(cassiere = get_best_queue(cl->casse, cl->k, NULL, -1), return NULL)
                    } else {    //Se ho già il puntatore alla cassa migliore
                        cassiere = better_cass;
                        better_cass = NULL;
                    }
                    MINUS1ERR(get_store_state(cl->store, &st_state), return NULL)
                    //Il supermercato è chiuso velocemente e non sono in coda quindi esco dal supermercato
                    if (st_state == closed_fast_state) break;
                    PTHERR(err, pthread_mutex_lock(&(cassiere)->mutex), return NULL)
                    MINUS1ERR(enqueued = join_queue(cassiere, clq, &queue_entrance), return NULL)
                    if (enqueued) {
                        if (change_queue) queue_counter++;
                        better_cass = NULL;
                        //Aspetto in coda ma esco se sono stato servito o se il supermercato chiude o se devo cambiare coda
                        MINUS1ERR(change_queue = wait_in_queue(cl, clq, cassiere, better_cass, &st_state), return NULL)
                        //Lascio la coda se il supermercato è in chiusura rapida o se devo cambiare coda
                        if (st_state == closed_fast_state || change_queue) {
                            leave_queue(cassiere, clq);
                        }
                    }
                    PTHERR(err, pthread_mutex_unlock(&(cassiere)->mutex), return NULL)
                } while(st_state != closed_fast_state && change_queue);
                //Sono fuori dalla coda quindi posso accedere senza lock
                if (clq->served) {
                    MINUS1ERR(time_in_queue = elapsed_time(&queue_entrance), return NULL)
                    DEBUG("[Thread Cliente %ld] Cliente %d: Sono stato servito\n", cl->id, client_id, clq->is_enqueued)
                }
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
    MINUS1ERR(destroy_client_in_queue(clq), return NULL)
    return stats;
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

static int get_products(client_t *cl) {
    unsigned int seed = cl->id + time(NULL);
    int products = RANDOM(seed, 0, cl->p);
    int time = RANDOM(seed, MIN_T, cl->t);
    MINUS1(msleep(time), return -1)
    return products;
}

static int wait_in_queue(client_t *cl, client_in_queue_t *clq, cassiere_t *cassiere, cassiere_t *better_cass, store_state *st_state) {
    int err, cost, change_queue = 0;

    //Se il cassiere mi sta processando, oppure se non sono stato servito e non devo cambiare cassa e lo stato
    //del supermercato non è in chiusura rapida allora aspetto in coda
    while(clq->processing || (*st_state != closed_fast_state && !change_queue && !clq->served)) {
        //Se il cassiere sta processando i miei prodotti allora aspetto che lui mi svegli
        if (clq->processing) {
            PTH(err, pthread_cond_wait(&(clq->waiting), &(cassiere->mutex)), return -1)
            PTH(err, pthread_mutex_unlock(&(cassiere)->mutex), return -1)
            MINUS1(get_store_state(cl->store, st_state), return -1)
            PTH(err, pthread_mutex_lock(&(cassiere)->mutex), return -1)
        } else {    //altrimenti attendo per un tot di secondi e faccio l'algoritmo di cambio cassa
            PTH(err, pthread_mutex_unlock(&(cassiere)->mutex), return -1)
            MINUS1(msleep(cl->s), return -1)
            MINUS1(get_store_state(cl->store, st_state), return -1)
            PTH(err, pthread_mutex_lock(&(cassiere)->mutex), return -1)
            //Se non sono stato servito e la cassa è aperta, faccio l'algoritmo di cambio cassa
            //Il supermercato deve essere aperto e il cassiere non deve processarmi
            if (*st_state != closed_fast_state && !clq->processing && !clq->served && cassiere->isopen) {
                MINUS1(cost = get_queue_cost(cassiere, clq), return -1)
                PTH(err, pthread_mutex_unlock(&(cassiere)->mutex), return -1)
                EQNULL(better_cass = get_best_queue(cl->casse, cl->k, cassiere, cost), return -1)
                MINUS1(get_store_state(cl->store, st_state), return -1)
                PTH(err, pthread_mutex_lock(&(cassiere)->mutex), return -1)
                //Se il supermercato non è chiuso velocemente e il cassiere non mi sta processando
                if (*st_state != closed_fast_state && !clq->processing) {
                    //Cambio coda se intanto la cassa è stata chiusa oppure se ho trovato una cassa migliore
                    change_queue = !cassiere->isopen || (!clq->served && cassiere != better_cass);
                }
            } else if (*st_state != closed_fast_state && !clq->processing && !clq->served && !cassiere->isopen) {
                change_queue = 1;
            }
        }
    }
    if (!change_queue) better_cass = NULL;

    return change_queue;
}
