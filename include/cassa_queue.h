/**
 * Gestione dedicata della coda di un cassiere. Si tratta di una double linked list di tipo FIFO con aggiunte e
 * rimozioni di complessità O(1). Alla coda sono associate una variabile che ne indica il costo ed una variabile che
 * indica il numero di clienti in coda. Durante le fasi di aggiunta o rimozione dalla coda, il costo della stessa ed il
 * numero di clienti viene aggiornato. Di conseguenza, per un cliente non in coda, conoscere costo e numero di clienti
 * in coda ha complessità O(1). Nel caso in cui un cliente in coda voglia conoscere il costo della cassa in relazione
 * alla sua posizione, la complessità è pari al numero di clienti che lo precedono in coda (ovvero che verranno serviti
 * prima) in quanto è necessario scorrere la lista partendo dalla posizione del cliente. Gli elementi aggiunti in coda
 * sono di tipo client_in_queue_t, quindi non viene esposta al cassiere la struttura dati del cliente ma solo ciò che
 * veramente è necessario (Information Hiding).
 */

#ifndef CASSA_QUEUE_H
#define CASSA_QUEUE_H
#include "storetypes.h"

/**
 * Crea la struttura dati della coda di un cassiere.
 *
 * @return la struttura dati creata oppure NULL in caso di errore ed imposta errno
 */
cassa_queue_t *cassa_queue_create(void);

/**
 * Distrugge la struttura dati della coda di un cassiere, liberandone la memoria. Dopo la chiamata di questa funzione,
 * il puntatore passato non deve essere più utilizzato in quanto punta ad un'area di memoria deallocata.
 *
 * @param q struttura dati della coda da distruggere
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno
 */
int cassa_queue_destroy(cassa_queue_t *q);

/**
 * Restituisce il costo della coda tenendo conto che il cliente si trova in coda. Avviene uno scorrimento della lista
 * partendo dal cliente in coda.
 *
 * @param cassiere in quale cassa il cliente è in coda
 * @param clq cliente in coda
 * @return costo della coda oppure -1 in caso di errore ed imposta errno
 */
int get_queue_cost(cassiere_t *cassiere, client_in_queue_t *clq);

/**
 * Restituisce il cliente successivo che deve essere servito rimuovendolo dalla coda.
 *
 * @param cassiere puntatore al cassiere
 * @param processing vale 1 se si vuole impostare il flag processing ad 1, 0 altrimenti
 * @return il cliente rimosso dalla testa della coda
 */
client_in_queue_t *get_next_client(cassiere_t *cassiere, int processing);

/**
 * Ritorna il puntatore al cassiere la cui coda ha il costo minimo valutando tutte le altre code ed escludendo la coda
 * in cui il cliente è accodato (se è accodato).
 *
 * @param cassieri array con tutti i cassieri del supermercato
 * @param k quanti cassieri ci sono in totale nel supermercato
 * @param from puntatore alla cassa in cui è accodato il cliente. Vale NULL se il cliente non è in coda
 * @param from_cost quanto costa stare nella coda in cui il cliente è accodato. Vale -1 se il cliente non è in coda
 * @return puntatore al migliore cassiere, NULL in caso di errore ed imposta errno
 */
cassiere_t *get_best_queue(cassiere_t **cassieri, int k, cassiere_t *from, int from_cost);

/**
 * Entra nella coda del cassiere specificato se la cassa è aperta. In fase di entrata viene preso l'istante in cui il
 * cliente si è accodato.
 *
 * @param cassiere puntatore al cassiere
 * @param clq puntatore al cliente
 * @param queue_entrance struttura dati nella quale salvare l'istante in cui il cliente è entrato in coda
 * @return 1 se l'entrata ha avuto successo, 0 altrimenti oppure -1 in caso di errore ed imposta errno
 */
int join_queue(cassiere_t *cassiere, client_in_queue_t *clq, struct timespec *queue_entrance);

/**
 * Rimuove il cliente specificato dalla coda del cassiere specificato.
 *
 * @param cassiere cassiere dal quale rimuovere il cliente
 * @param clq cliente che deve lasciare la coda
 */
void leave_queue(cassiere_t *cassiere, client_in_queue_t *clq);

#endif //CASSA_QUEUE_H
