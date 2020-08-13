#ifndef CLIENT_H
#define CLIENT_H

#include "storetypes.h"

/**
 * Funzione svolta dal thread cliente. Il thread, dopo aver simulato il cliente all'interno del supermercato, viene
 * riutilizzato, quindi non è necessario lanciare un nuovo thread cliente ogni volta che un cliente entra nel
 * supermercato.
 *
 * @param args argomenti della funzione
 * @return statistiche in caso di terminazione con successo, NULL in caso di errore
 */
void *client_thread_fun(void *args);

/**
 * Alloca la struttura dati del thread cliente. Questa struttura viene passata al thread come argomento e contienere
 * variabili fondamentali per il ciclo di vita del thread cliente.
 *
 * @param id identificatore univoco del thread
 * @param store struttura dati del supermercato
 * @param t massimo tempo (in millisecondi) per l'acquisto di prodotti
 * @param p massimo numero di prodotti acquistabili da un cliente
 * @param s ogni quanto tempo (in millisecondi) il cliente decide se cambiare cassa o meno
 * @param k quante casse sono presenti nel supermercato
 * @param fd descrittore del file utilizzato per la comunicazione delle richieste di uscita dal supermercato
 * @param fd_mtx puntatore alla mutex da utilizzare per svolgere le operazioni in mutua esclusione sul file descriptor
 * @return struttura dati oppure NULL in caso di errore ed imposta errno
 */
client_t *alloc_client(size_t id, store_t *store, cassiere_t **casse, int t, int p, int s, int k);

/**
 * Distrugge la struttura dati del cliente, liberandone la memoria occupata. Dopo la chiamata di questa funzione, il
 * puntatore perde di significato e non deve essere assolutamente riutilizzato.
 *
 * @param client puntatore alla struttura dati di cui liberare la memoria
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno
 */
int client_destroy(client_t *client);

/**
 * Imposta lo stato di uscita del cliente, ovvero se il cliente ha ricevuto il permesso per uscire dal supermercato.
 *
 * @param client struttura dati del cliente
 * @param can_exit vale 1 se il cliente ha il permesso per uscire dal supermercato, 0 altrimenti
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno
 */
int set_exit_permission(client_t *client, int can_exit);

/**
 * Rimane il attesa che arrivi una risposta alla richiesta di uscita dal supermercato. Chiamata bloccante.
 *
 * @param cl struttura dati del cliente
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno
 */
int wait_permission(client_t *cl);

int enter_best_queue(client_t *cl, client_in_queue_t *clq, struct timespec *queue_entrance);

int wait_to_be_served(int s, client_in_queue_t *clq, store_state *st_state);

#endif //CLIENT_H
