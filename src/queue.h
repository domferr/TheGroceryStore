/**
 * Header file per una unbounded queue FIFO. La struttura dati è una double linked list. Ogni nodo è rappresentato dalla
 * struct node_t. Ogni nodo possiede un elemento generico, il puntatore al suo successore ed il puntatore al nodo che
 * lo precede. Quando non vi è un successore, il puntatore del successore è nullo. Quando non vi è un precedessore,
 * il puntatore del predecessore è nullo. La coda fornisce operazioni di aggiunta (push) e rimozione (pop).
 * Le operazioni NON sono thread safe in quanto non vengono svolte in mutua esclusione.
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
 * Struttura che rappresenta la coda stessa.
 */
typedef struct {
    node_t *head;           //puntatore alla testa della coda
    node_t *tail;           //puntatore alla coda della coda
    int size;               //numero di elementi presenti nella coda
} queue_t;

/**
 * Crea una coda. Ritorna un puntatore alla coda se la creazione è avvenuta con successo, NULL altrimenti.
 *
 * @return puntatore ad una coda oppure NULL in caso di errore
 */
queue_t *queue_create(void);

/**
 * Distrugge una coda liberandone la memoria in uso. Dopo la chiamata di questa funzione, la coda non può essere
 * riutilizzata ed il suo puntatore avrà perso di significato in quanto la memoria allocata per la coda viene totalmente
 * ripulita. Ritorna 0 se la distruzione è avvenuta con successo, -1 in caso di errore.
 *
 * @param queue coda da distruggere
 * @return 0 in caso di successo, -1 altrimenti
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
int push(queue_t *queue, void *elem);

/**
 * Rimuove dalla coda l'elemento in coda e lo ritorna al chiamante. Ritorna NULL se la coda è vuota.
 *
 * @param queue coda dalla quale rimuovere la coda
 * @return elemento che si trovava in coda, NULL in caso di coda vuota
 */
void *pop(queue_t *queue);

int foreach(queue_t *queue, int (*fun)(void*, void*), void *args);

/**
 * Elimina tutti gli elementi dalla coda e dalla memoria. Dopo la chiamata di questa funzione, la coda ha 0 elementi.
 *
 * @param queue coda da svuotare
 */
void clear(queue_t *queue);

#endif //QUEUE_H
