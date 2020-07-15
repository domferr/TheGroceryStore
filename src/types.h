#ifndef SALVATORE_H
#define SALVATORE_H

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t waiting;
    queue_t *queue;
} manager_queue;

typedef enum {
    active,    //Cassa APERTA. Continua a servire i clienti in coda.
    sleeping   //Cassa CHIUSA. Servi il cliente e non servire gli altri clienti in coda. Rimani in attesa di essere riattivato.
} cashier_state;

typedef struct {
    cashier_state state;        //stato del cassiere
    pthread_mutex_t mutex;
    pthread_cond_t paused;      //il cassiere attende su questa condition variable quando la cassa non è attiva
    pthread_cond_t noclients;   //il cassiere attende su questa condition variable quando non ci sono clienti in coda
} cashier_sync;

typedef struct {
    size_t id;                  //identificatore univoco di questo cassiere tra tutti i cassieri
    grocerystore_t *gs;
    queue_t *queue;             //clienti in coda
    cashier_sync *ca_sync;
    int product_service_time;   //quanto impiega a gestire un singolo prodotto
    int fixed_service_time;     //tempo fisso per la gestione di un cliente
    manager_queue *mq;
    int interval;   //intervallo tra una notifica ed un'altra. espresso in millisecondi
} cashier_t;

typedef enum {
    turned_on,
    turned_off,
    stopped
} notifier_state;

typedef struct {
    pthread_t thread;
    cashier_t *ca;
    pthread_cond_t paused;
    notifier_state state;
    pthread_mutex_t mutex;
} notifier_t;

typedef struct {
    size_t id;    //identificatore univoco della cassa
    int clients_in_queue;   //numero di clienti in coda in cassa
} notification_data;

#endif //THEGROCERYSTORE_SALVATORE_H
