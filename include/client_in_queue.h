#ifndef CLIENT_IN_QUEUE_H
#define CLIENT_IN_QUEUE_H

#include "storetypes.h"

/**
 * Crea la struttura dati del cliente in coda e la ritorna al chiamante.
 *
 * @return struttura dati del cliente in coda oppure NULL in caso di errore ed imposta errno
 */
client_in_queue_t *alloc_client_in_queue(void);

/**
 * Distrugge la struttura dati del cliente in coda passata per argomento. Dopo la chiamata di questa funzione, il puntatore
 * non deve essere più utilizzato in quanto perde di significato e punta ad un'area deallocata.
 *
 * @param clq struttura dati da distruggere e da liberare la memoria
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno
 */
int destroy_client_in_queue(client_in_queue_t *clq);

/**
 * Sveglia il cliente in coda passato per argomento. Imposta anche se è stato servito o meno in base al parametro passato
 * alla funzione. Se il parametro vale 1, allora il cliente viene svegliato notificandogli che è stato servito. Se il
 * parametro vale 0 allora il cliente viene svegliato e notificato che non è stato servito. Poichè per accedere al cliente
 * in coda bisogna avere la lock della coda, è necessario chiamare questa funzione se e solo se si ha la lock del cassiere.
 * Altrimenti la chiamata di questa funzione non è garantita essere thread safe.
 *
 * @param clq cliente in coda
 * @param served vale 1 se il cliente viene svegliato perchè è stato servito, oppure 0 in caso contrario
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno
 */
int wakeup_client(client_in_queue_t *clq, int served);

#endif //CLIENT_IN_QUEUE_H
