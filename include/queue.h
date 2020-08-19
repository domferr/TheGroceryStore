/**
 * Header file per una unbounded queue FIFO. La struttura dati è una double linked list. Ogni nodo è rappresentato dalla
 * struct node_t. Ogni nodo possiede un elemento generico, il puntatore al suo successore ed il puntatore al nodo che
 * lo precede. Quando non vi è un successore, il puntatore del successore è nullo. Quando non vi è un precedessore,
 * il puntatore del predecessore è nullo. La coda fornisce operazioni di aggiunta (push) e rimozione (pop).
 * Le operazioni NON sono thread safe in quanto non vengono svolte in mutua esclusione.
 */

#ifndef QUEUE_H
#define QUEUE_H

/**
 * Struttura che rappresenta un nodo della coda
 */
typedef struct node {
    void *elem;             //elemento generico contenuto nel nodo
    struct node *next;      //puntatore al nodo successivo
    struct node *prev;      //puntatore al nodo precedente
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
 * @param free_fun funzione utilizzata per liberare la memoria occupata da ogni elemento in coda. Vale NULL se non si vuole
 * distruggere gli elementi in coda ma se si vuole soltanto distruggere la coda e la sua struttura dati
 * @return 0 in caso di successo, -1 altrimenti
 */
int queue_destroy(queue_t *queue, void (*free_fun)(void *elem));

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

/**
 * Applica la funzione passata per argomento ad ogni elemento della coda
 *
 * @param queue la coda sulla quale applicare la funzione
 * @param fun la funzione da applicare ad ogni elemento. Il primo parametro passato è l'elemento in coda, il secondo è
 * invece un parametro aggiuntivo a discrezione
 * @param args argomenti aggiuntivi da passare alla funzione per ogni chiamata (oltre all'elemento della coda)
 * @return 0 se l'applicazione è avvenuta con successo, -1 altrimenti e imposta errno
 */
int foreach(queue_t *queue, int (*fun)(void*, void*), void *args);

/**
 * Elimina tutti gli elementi dalla coda e dalla memoria. Dopo la chiamata di questa funzione, la coda ha 0 elementi.
 *
 * @param queue coda da svuotare
 * @param free_fun funzione utilizzata per liberare la memoria occupata da ogni elemento in coda. Vale NULL se non si vuole
 * distruggere gli elementi in coda ma se si vuole soltanto distruggere la coda e la sua struttura dati
 */
void clear(queue_t *queue, void (*free_fun)(void *elem));

/**
 * Appende la seconda coda nella prima. La seconda coda viene accodata alla prima e ne viene liberata la memoria. Dopo
 * la chiamata di questa funzione, il puntatore della seconda coda perde di significato e punta ad un'area di memoria
 * non allocata quindi non deve essere utilizzato.
 *
 * @param q1 prima coda alla quale appendere la seconda coda
 * @param q2 seconda coda
 */
void merge(queue_t *q1, queue_t *q2);

#endif //QUEUE_H
