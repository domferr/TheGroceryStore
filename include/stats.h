#ifndef STATS_H
#define STATS_H

/** Statistiche di un cliente entrato nel supermercato */
typedef struct {
    int id;             //identificatore univoco del cliente
    int products;       //numero di prodotti acquistati
    long time_in_store; //tempo di permanenza nel supermercato espresso in millisecondi
    long time_in_queue; //tempo di attesa in coda espresso in millisecondi
    int queue_counter;  //numero di code visitate
} client_stats;

/**
 * Crea la struttura dati di un cliente entrato nel supermercato identificato dall'intero passato per argomento.
 *
 * @param id identificatore univoco del cliente
 * @return la struttura dati creata oppure NULL in caso di errore ed imposta errno
 */
client_stats *new_client_stats(int id);

#endif //STATS_H
