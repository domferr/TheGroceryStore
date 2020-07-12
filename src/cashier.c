#include "utils.h"
#include "grocerystore.h"
#include "cashier.h"
#include "queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

#define DEBUG_CASHIER
#define MIN_SERVICE_TIME 20 //ms
#define MAX_SERVICE_TIME 80 //ms

int serve_client(client_in_queue *client, cashier_t *ca, int fixed_service_time);
void handle_closure(void);

void *cashier_fun(void *args) {
    int err;
    cashier_t *ca = (cashier_t*) args;
    PTH(err, pthread_mutex_lock(&(ca->mutex)), return NULL)
    cashier_state internal_state = ca->state;
    queue_t *queue = ca->queue;
    unsigned int seed = ca->id;
    PTH(err, pthread_mutex_unlock(&(ca->mutex)), return NULL)
    int fixed_service_time = RANDOM(seed, MIN_SERVICE_TIME, MAX_SERVICE_TIME);  //milliseconds

    while(internal_state != closing) {
        //Attendo che la cassa sia aperta
        PTH(err, pthread_mutex_lock(&(ca->mutex)), return NULL)
        while (ca->state == pause) {
            PTH(err, pthread_cond_wait(&(ca->paused), &(ca->mutex)), return NULL)
        }
        internal_state = ca->state;
        PTH(err, pthread_mutex_unlock(&(ca->mutex)), return NULL)
        //Se la cassa è aperta
        if (internal_state == run) {
            //prendo il cliente dalla coda
            client_in_queue *client = (client_in_queue *) removeFromEnd(queue);
            EQNULL(client, perror("remove from queue"); return NULL)

            PTH(err, pthread_mutex_lock(&(ca->mutex)), return NULL)
            internal_state = ca->state;
            PTH(err, pthread_mutex_unlock(&(ca->mutex)), return NULL)

            switch(internal_state) {
                case pause:
                    //TODO sveglia tutti gli altri clienti e servi il cliente client
                    //TODO for each in queue do wake_client(client, 0);
                    serve_client(client, ca, fixed_service_time);
                    break;
                case closing:
                    //TODO valuta se devi servire tutti gli altri o meno
                    handle_closure();
                    break;
                case run:   //Servo il cliente corrente
                    serve_client(client, ca, fixed_service_time);
                    break;

            }
        } else if (internal_state == closing) {  //Se il supermercato è chiuso
            handle_closure();
        }

    }
    printf("Cassiere %ld: termino\n", ca->id);
    cashier_free(ca);
    return 0;
}

void handle_closure(void) {
    //TODO wake up all clients
}

int serve_client(client_in_queue *client, cashier_t *ca, int fixed_service_time) {
    int err, ms;
    PTH(err, pthread_mutex_lock(&(client->mutex)), return -1)
    ms = fixed_service_time + (ca->product_service_time)*client->products;
    PTH(err, pthread_mutex_unlock(&(client->mutex)), return -1)
    printf("Cassiere %ld: Servo un cliente. Impiegherò %dms\n", ca->id, ms);
    msleep(ms);
    wake_client(client, 1);
    return 0;
}

void wake_client(client_in_queue *client, int done) {
    pthread_mutex_lock(&(client->mutex));
    client->done = done;
    pthread_cond_signal(&(client->waiting));
    pthread_mutex_unlock(&(client->mutex));
}

cashier_t *alloc_cashier(size_t id, grocerystore_t *gs, cashier_state starting_state, int product_service_time) {
    int err;
    cashier_t *ca = (cashier_t*) malloc(sizeof(cashier_t));
    EQNULL(ca, return NULL)

    ca->id = id;
    ca->gs = gs;
    ca->product_service_time = product_service_time;
    ca->state = starting_state;
    ca->queue = queue_create();
    EQNULL(ca->queue, return NULL)

    PTH(err, pthread_mutex_init(&(ca->mutex), NULL), return NULL)

    PTH(err, pthread_cond_init(&(ca->paused), NULL), return NULL)

    return ca;
}

void cashier_free(cashier_t *ca) {
    pthread_mutex_destroy(&(ca->mutex));
    pthread_cond_destroy(&(ca->paused));
    queue_destroy(ca->queue);
    free(ca);
}