#ifndef CLIENT_IN_QUEUE_H
#define CLIENT_IN_QUEUE_H

#include <pthread.h>

typedef enum {
    waiting,    //Il cliente è in attesa di essere servito. La cassa ed il supermercato sono aperti
    done,       //Il cliente è stato servito dal cassiere
    store_closure,  //Il cliente non è stato servito perchè il supermercato è in chiusura e deve andarsene senza fare acquisti
    cashier_sleeping    //La cassa è stata chiusa. Il cliente deve cercare una nuova cassa
} client_status;

/**
 * Struttura dati utilizzata per interfacciare cassieri e clienti. Utile per information hiding in quanto il cliente
 * non può accedere direttamente al cassiere ed il cassiere può accedere solo a ciò che è per lui fondamentale, ovvero
 * la lock e la condition variable sulla quale il cliente sta attendendo mentre si trova in coda.
 */
typedef struct {
    size_t id;
    int products;   //prodotti acquistati
    pthread_mutex_t mutex;
    pthread_cond_t waiting; //variabile di condizione sulla quale il cliente aspetta di essere servito
    client_status status;   //vale done se è stato servito
} client_in_queue;

client_in_queue *alloc_client_in_queue(size_t id);
int destroy_client_in_queue(client_in_queue *cl_in_q);

#endif //CLIENT_IN_QUEUE_H
