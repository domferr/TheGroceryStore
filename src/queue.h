/**
 * Header file per una unbounded queue. La struttura dati è una double linked list. Ogni nodo è rappresentato dalla
 * struct node_t. Ogni nodo possiede un elemento generico, il puntatore al suo successore ed il puntatore al nodo che
 * lo precede. Quando non vi è un successore, il puntatore del successore è nullo. Quando non vi è un precedessore,
 * il puntatore del predecessore è nullo. La coda fornisce operazioni di aggiunta, rimozione e scorrimento. Tutte le
 * operazioni vengono svolte in mutua esclusione.
 *
 * Esempio di coda FIFO: aggiungere in coda con la funzione addAtStart() e prendere dalla coda con la funzione removeFromEnd()
 * Esempio di coda LIFO: aggiungere in coda con la funzione addAtStart() e prendere dalla coda con la funzione removeFromStart()
 */

#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

/**
 * Struttura che rappresenta un nodo della coda
 */
typedef struct node_t {
    void *elem;             //elemento generico contenuto nel nodo
    struct node_t *next;    //puntatore al nodo successivo
    struct node_t *prev;    //puntatore al nodo precedente
} node_t;

/**
 * Struttura che rappresenta la coda stessa. Contiene la mutex utilizzata per svolgere le operazioni in mutua esclusione.
 */
typedef struct {
    pthread_mutex_t mtx;    //mutex usata per ottenere la mutua esclusione
    node_t *head;           //puntatore alla testa della coda
    node_t *tail;           //puntatore alla coda della coda
    int size;               //numero di elementi presenti nella coda
    pthread_cond_t empty;
} queue_t;

/**
 * Crea una coda. Ritorna un puntatore alla coda se la creazione è avvenuta con successo, NULL altrimenti.
 *
 * @return puntatore ad una coda oppure NULL in caso di errore
 */
queue_t *queue_create(void);

/**
 * Coda da distruggere. Dopo la chiamata di questa funzione, la coda non può essere riutilizzata ed il suo puntatore
 * avrà perso di significato in quanto la memoria allocata per la coda viene totalmente ripulita.
 *
 * @param queue coda da distruggere
 */
int queue_destroy(queue_t *queue);

/**
 * Aggiunge in testa alla coda passata per argomento l'elemento specificato.
 * Ritorna 0 se l'aggiunta è avvenuta con successo, -1 in caso di errore.
 *
 * @param queue coda da modificare
 * @param elem elemento da aggiungere in testa
 * @return 0 se l'aggiunta è avvenuta con successo, -1 altrimenti.
 */
int addAtStart(queue_t *queue, void *elem);

/**
 * Aggiunge alla fine della coda passata per argomento l'elemento specificato.
 * Ritorna 0 se l'aggiunta è avvenuta con successo, -1 in caso di errore.
 *
 * @param queue coda da modificare
 * @param elem elemento da aggiungere in coda
 * @return 0 se l'aggiunta è avvenuta con successo, -1 altrimenti.
 */
int addAtEnd(queue_t *queue, void *elem);

/**
 * Rimuove dalla coda l'elemento in testa e lo ritorna al chiamante.
 *
 * @param queue coda dalla quale rimuovere la testa
 * @return elemento che si trovava in testa, NULL in caso di errore
 */
void *removeFromStart(queue_t *queue);

/**
 * Rimuove dalla coda l'ultimo elemento e lo ritorna al chiamante.
 *
 * @param queue coda dalla quale rimuovere l'ultimo elemento
 * @return elemento che si trovava in coda, NULL in caso di errore
 */
void *removeFromEnd(queue_t *queue);

/**
 * Elimina tutti gli elementi dalla coda e dalla memoria. Dopo la chiamata di questa funzione, la coda ha 0 elementi.
 * @param queue coda da azzerare
 * @return 0 in caso di successo, -1 altrimenti
 */
int clear(queue_t *queue);

/**
 * Partendo dalla testa e scorrendo la coda fino alla sua coda, chiama la funzione passata per argomento.
 * Alla funzione viene passato l'elemento stesso.
 *
 * @param queue coda da scorrere dalla testa fino alla coda
 * @param jobFun funzione da chiamare per ogni elemento
 * @return 0 in caso di successo, -1 altrimenti
 */
int applyFromFirst(queue_t *queue, void (*jobFun)(void*));

/**
 * Partendo dalla coda e scorrendo la coda fino alla sua testa, chiama la funzione passata per argomento.
 * Alla funzione viene passato l'elemento stesso.
 *
 * @param queue coda da scorrere dalla coda fino alla testa
 * @param jobFun funzione da chiamare per ogni elemento
 * @return 0 in caso di successo, -1 altrimenti
 */
int applyFromLast(queue_t *queue, void (*jobFun)(void*));

#endif //QUEUE_H
