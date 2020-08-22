/**
 * Header file per una unbounded double linked list FIFO. Ogni nodo è rappresentato dalla struct node_t. Ogni nodo
 * possiede un elemento generico, il puntatore al suo successore ed il puntatore al nodo che lo precede. Quando non vi
 * è un successore, il puntatore del successore è nullo. Quando non vi è un precedessore, il puntatore del predecessore
 * è nullo. La lista fornisce operazioni di aggiunta (push) e rimozione (pop). E' possibile accodare due liste con la
 * funzione append. Per ordinare e fondere due o k liste vengono fornite le funzioni merg_sort e mergesort_k_lists, le
 * quali svolgono l'algoritmo mergesort rispettivamente per due liste e per k liste.
 * Le operazioni NON sono thread safe in quanto non vengono svolte in mutua esclusione.
 */

#ifndef LIST_H
#define LIST_H

/**
 * Struttura che rappresenta un nodo della lista
 */
typedef struct node {
    void *elem;             //elemento generico contenuto nel nodo
    struct node *next;      //puntatore al nodo successivo
    struct node *prev;      //puntatore al nodo precedente
} node_t;

/**
 * Struttura che rappresenta la lista stessa.
 */
typedef struct {
    node_t *head;           //puntatore alla testa della lista
    node_t *tail;           //puntatore alla coda della lista
    int size;               //numero di elementi presenti nella lista
} list_t;

/**
 * Crea una lista. Ritorna un puntatore alla lista se la creazione è avvenuta con successo, NULL altrimenti.
 *
 * @return puntatore ad una lista oppure NULL in caso di errore
 */
list_t *list_create(void);

/**
 * Distrugge una lista liberandone la memoria in uso. Dopo la chiamata di questa funzione, la lista non può essere
 * riutilizzata ed il suo puntatore avrà perso di significato in quanto la memoria allocata per la lista viene totalmente
 * ripulita. Ritorna 0 se la distruzione è avvenuta con successo, -1 in caso di errore.
 *
 * @param list lista da distruggere
 * @param free_fun funzione utilizzata per liberare la memoria occupata da ogni elemento in lista. Vale NULL se non si vuole
 * distruggere gli elementi in lista ma se si vuole soltanto distruggere la lista e la sua struttura dati
 * @return 0 in caso di successo, -1 altrimenti
 */
int list_destroy(list_t *list, void (*free_fun)(void*));

/**
 * Aggiunge in testa alla lista passata per argomento l'elemento specificato.
 * Ritorna 0 se l'aggiunta è avvenuta con successo, -1 in caso di errore.
 *
 * @param list lista da modificare
 * @param elem elemento da aggiungere in testa
 * @return 0 se l'aggiunta è avvenuta con successo, -1 altrimenti.
 */
int push(list_t *list, void *elem);

/**
 * Rimuove dalla lista l'elemento in coda e lo ritorna al chiamante. Ritorna NULL se la lista è vuota.
 *
 * @param list lista dalla quale rimuovere la coda
 * @return elemento che si trovava nella lista, NULL in caso di lista vuota
 */
void *pop(list_t *list);

/**
 * Applica la funzione passata per argomento ad ogni elemento della lista.
 *
 * @param list la lista sulla quale applicare la funzione
 * @param fun la funzione da applicare ad ogni elemento. Il primo parametro passato è l'elemento in lista, il secondo è
 * invece un parametro aggiuntivo a discrezione
 * @param args argomenti aggiuntivi da passare alla funzione per ogni chiamata (oltre all'elemento della lista)
 * @return 0 se l'applicazione è avvenuta con successo, -1 altrimenti e imposta errno
 */
int foreach(list_t *list, int (*fun)(void*, void*), void *args);

/**
 * Elimina tutti gli elementi dalla lista e dalla memoria. Dopo la chiamata di questa funzione, la lista ha 0 elementi.
 *
 * @param list lista da svuotare
 * @param free_fun funzione utilizzata per liberare la memoria occupata da ogni elemento nella lista. Vale NULL se non si vuole
 * distruggere gli elementi ma se si vuole soltanto distruggere la lista e la sua struttura dati
 */
void clear(list_t *list, void (*free_fun)(void*));

/**
 * Appende la seconda lista nella prima. La seconda lista viene accodata alla prima e ne viene liberata la memoria. Dopo
 * la chiamata di questa funzione, il puntatore della seconda lista perde di significato e punta ad un'area di memoria
 * non allocata quindi non deve essere utilizzato.
 *
 * @param l1 prima lista alla quale appendere la seconda lista
 * @param l2 seconda lista
 */
void append(list_t *l1, list_t *l2);

/**
 * Esegue l'algoritmo merge sort. Le due liste passate per argomento devono essere ordinate in senso crescente, ovvero
 * che il primo elemento aggiunto è minore del secondo aggiunto e così via. La lista risultante è la lista puntata dal
 * puntatore first. Il puntatore second perde di significato a seguito della chiamata di questa funzione e non deve
 * essere riutilizzato in quanto viene liberata la memoria occupata dalla lista puntata da second. Ritorna 0 in caso di
 * successo, -1 altrimenti ed imposta errno.
 *
 * @param first prima lista
 * @param second seconda lista
 * @param compare funzione utilizzata per comparare gli elementi delle due liste tra loro
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno
 */
int mergesort(list_t *first, list_t *second, int (*compare)(void*, void*));

/**
 * Fonde in una sola lista k liste passate per argomento. Ognuna delle k liste deve essere ordinata. Ritorna la lista
 * risultante oppure NULL in caso di errore. Dopo la chiamata di questa funzione i puntatori alle k liste non devono
 * essere riutilizzati perchè perdono di significato e deve essere usato solo il puntatore ritornato.
 *
 * @param lists array di k liste
 * @param k quante liste devono essere fuse
 * @param compare funzione da utilizzare per confrontare due elementi della lista. Ritorna un valore meno di zero oppure
 * uguale a zero oppure maggiore di zero rispettivamente se il primo valore è minore oppure uguale oppure maggiore del
 * secondo valore
 * @return lista risultante oppure NULL in caso di errore
 */
list_t *mergesort_k_lists(list_t **lists, int k, int (*compare)(void*, void*));

#endif //LIST_H
