#ifndef GROCERYSTORE_H
#define GROCERYSTORE_H

#define _POSIX_C_SOURCE 200809L
#include <pthread.h>

#define ISOPEN(st) st == open
#define ISCLOSED(st) st != open

typedef enum {
    open,       //Il supermercato è aperto. I clienti possono entrare
    closed,     //Il supermercato è chiuso. I clienti non possono entrare mentre quelli già dentro devono finire gli acquisti
    closed_fast //Il supermercato è chiuso. I clienti non possono entrare e quelli dentro devono subito uscire
} gs_state;

typedef struct {
    pthread_mutex_t mutex;
    gs_state state; //stato del supermercato
    int clients_inside; //numero di clienti dentro al supermercato
    pthread_cond_t exit;        //Il gestore delle entrate rimane in attesa su questa condition variable per contare le uscite e fare entrare nuovi clienti
    pthread_cond_t entrance;    //I thread clienti rimangono in attesa su questa condition variable. Vengono svegliati quando ci sono C-E clienti dentro al supermercato
    int can_enter;      //vale tanto quanto il numero di clienti che possono entrare.
    int total_clients;  //contatore per dare un nuovo identificatore ai clienti che entrano nel supermercato
} grocerystore_t;

/**
 * Crea la struttura dati dello store. Richiede il numero massimo di clienti che possono esserci contemporaneamente
 * all'interno del supermercato.
 *
 * @param c numero massimo di clienti che possono esserci contemporaneamente all'interno del supermercato
 * @return puntatore alla struttura dati allocata, NULL in caso di errore e setta errno
 */
grocerystore_t *grocerystore_create(size_t c);

/**
 * Distrugge la struttura del supermercato passata per argomento. Il puntatore passato, dopo l'esecuzione di questa
 * funzione non ha più utilizzato. La memoria viene totalmente liberata
 *
 * @param gs struttura dati del supermercato di cui liberare la memoria
 * @return 0 se la distruzione è avvenuta con successo, -1 altrimenti e setta errno
 */
int grocerystore_destroy(grocerystore_t *gs);


int enter_store(grocerystore_t *gs, gs_state *state);
int exit_store(grocerystore_t *gs);
int manage_entrance(grocerystore_t *gs, gs_state *state, int c, int e);
int get_store_state(grocerystore_t *gs, gs_state *state);

#endif //GROCERYSTORE_H