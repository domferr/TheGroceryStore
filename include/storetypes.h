#ifndef STORETYPES_H
#define STORETYPES_H

#include "queue.h"
#include <pthread.h>

typedef enum {
    open_state,       //Il supermercato è aperto. I clienti possono entrare
    closed_state,     //Il supermercato è chiuso. I clienti non possono entrare mentre quelli già dentro devono finire gli acquisti
    closed_fast_state //Il supermercato è chiuso. I clienti non possono entrare e quelli dentro devono subito uscire
} store_state;

typedef struct {
    pthread_cond_t entrance;    //I thread clienti rimangono in attesa su questa condition variable. Vengono svegliati quando ci sono C-E clienti dentro al supermercato
    int clients_outside;        //numero di clienti usciti dal supermercato
    int group_size;     //parametro e
    int can_enter;          //vale tanto quanto il numero di clienti che possono entrare
} store_t;

typedef enum {
    cassa_open_state,   //la cassa è aperta
    cassa_closed_state  //la cassa è chiusa
} cassiere_state;

typedef struct {
    store_t *store;
    size_t id;                  //identificatore univoco del cassiere
    cassiere_state state;       //stato del cassiere
    queue_t *queue;             //clienti in coda
    pthread_mutex_t mutex;
    pthread_cond_t noclients;   //il cassiere aspetta su questa variabile di condizione quando non ci sono clienti
    pthread_cond_t waiting;     //il cassiere attende su questa variabile di condizione quando la cassa viene chiusa
    int product_service_time;   //quanto impiega a gestire un singolo prodotto
    int fixed_service_time;     //tempo fisso per la gestione di un cliente
    int interval;   //intervallo tra una notifica ed un'altra. espresso in millisecondi
} cassiere_t;

typedef struct {
    size_t id;
    pthread_mutex_t mutex;
    pthread_cond_t exit_permission;  //Il thread cliente rimane attesa su questa condition variable per aspettare il permesso di uscita
    int can_exit;
    store_t *store;
    cassiere_t **casse;
    int k;  //massimo numero di casse del supermercato
    int t;  //tempo massimo per acquistare prima di mettersi in una coda
    int p;  //numero massimo di prodotti che acquista
    int s;  //ogni quanto tempo il cliente decide se cambiare cassa
} client_t;

/**
 * Struttura dati utilizzata per interfacciare cassieri e clienti. Utile per information hiding in quanto il cliente
 * non può accedere direttamente al cassiere ed il cassiere può accedere solo a ciò che è per lui fondamentale, ovvero
 * la lock e la condition variable sulla quale il cliente sta attendendo mentre si trova in coda.
 */
typedef struct {
    pthread_mutex_t *mutex;
    pthread_cond_t waiting; //variabile di condizione sulla quale il cliente aspetta di essere servito
    int products;           //numero di prodotti acquistati
    int served;             //vale 1 se è stato servito, 0 altrimenti
} client_in_queue_t;

#endif //STORETYPES_H
