/**
 * Un thread pool è una struttura dati che contiene un insieme di thread di dimensione fissa. E' possibile aggiungere un
 * nuovo thread se nel pool c'è almeno un posto libero ed è possibile attendere che tutti i thread finiscano il loro lavoro.
 * Un thread, quando creato, esegue una funzione specificata e con argomenti specificati. Tutte le operazioni vengono
 * eseguite in mutua esclusione.
 */

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>

typedef struct {
    size_t max_size;    //numero massimo di thread nel pool
    size_t size;        //numero di thread nel pool
    pthread_t *threads; //array di threads
    pthread_mutex_t mtx;
} thread_pool_t;

/**
 * Crea un thread pool con dimensione massima specificata.
 *
 * @param max_size massimo numero di elementi che può avere il thread pool
 * @return puntatore al thread pool creato oppure NULL in caso di errore
 */
thread_pool_t *thread_pool_create(int max_size);

/**
 * Attende che tutti i thread del pool termino. Esegue pthread_join() su ogni thread del thread pool. Chiamata bloccante.
 * Il controllo viene passato al chiamante solo quando tutti i thread del thread pool hanno finito la loro esecuzione.
 *
 * @param pool l'attesa viene eseguita sugli elementi di questo thread pool
 */
void** thread_pool_join(thread_pool_t *pool);   //TODO update documentation

/**
 * Libera la memoria occupata dal thread pool passato per argomento. Quando il controllo viene passato al chiamante, il
 * puntatore al thread pool non ha più significato.
 *
 * @param pool thread pool che deve essere rimosso dalla memoria
 */
int thread_pool_free(thread_pool_t *pool);  //TODO update documentation

/**
 * Crea un nuovo thread nel thread pool che eseguirà la funzione specificata (passando gli argomenti specificati).
 * Ritorna 0 se la creazione avviene con successo, -1 altrimenti. Il nuovo thread viene creato se e solo se c'è abbastanza
 * spazio nel thread pool.
 *
 * @param pool thread pool nel quale aggiungere il nuovo thread
 * @param fun la funzione che il nuovo thread deve eseguire
 * @param args gli argomenti della funzione che il nuovo thread deve eseguire
 * @return 0 in caso di successo, -1 altrimenti
 */
int thread_create(thread_pool_t *pool, void *(*fun)(void *), void *args);

#endif //THREADPOOL_H