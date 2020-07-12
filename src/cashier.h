#ifndef CASHIER_H
#define CASHIER_H

#include "queue.h"
#include "grocerystore.h"

typedef enum {
    active,    //Cassa APERTA. Continua a servire i clienti in coda.
    sleeping   //Cassa CHIUSA. Servi il cliente e non servire gli altri clienti in coda. Rimani in attesa di essere riattivato.
} cashier_state;

typedef struct {
    size_t id;                  //identificatore univoco di questo cassiere tra tutti i cassieri
    grocerystore_t *gs;
    queue_t *queue;             //clienti in coda
    cashier_state state;        //stato del cassiere
    int product_service_time;   //quanto impiega a gestire un singolo prodotto
    pthread_mutex_t mutex;
    pthread_cond_t paused;
} cashier_t;

typedef struct {
    int products;
    pthread_mutex_t mutex;
    pthread_cond_t waiting;
    int done;
} client_in_queue;

void *cashier_fun(void *args);
cashier_t *alloc_cashier(size_t id, grocerystore_t *gs, cashier_state starting_state, int product_service_time);
int cashier_wait_activation(cashier_t *ca, grocerystore_t *gs, cashier_state *state);
int cashier_get_client(cashier_t *ca, client_in_queue **client, cashier_state *ca_state);
int serve_client(cashier_t *ca, client_in_queue *client, cashier_state *state, int fixed_service_time);
void wake_client(client_in_queue *client, int done);

#endif //CASHIER_H
