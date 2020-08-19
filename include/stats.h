#ifndef STATS_H
#define STATS_H

#include <stdio.h>
#include "queue.h"

/** Statistiche di una cassa del supermercato */
typedef struct {
    size_t id;
    queue_t *served;
    queue_t *opened;
    int closed_counter;
    int products_counter;
} cassa_log_t;

/**
 * Aggiorna il log del cliente con i parametri passati. Ritorna 0 in caso di successo, -1 altrimenti.
 *
 * @param client_log struttura dati del
 * @param id identificatore univoco del cliente
 * @param prod quanti prodotti ha acquistato il cliente
 * @param time_instore quanto tempo il cliente ha passato all'interno del supermercato. Espresso in millisecondi
 * @param time_inqueue quanto tempo il cliente ha passato in coda. Espresso in millisecondi
 * @param queuecounter quante volte il cliente ha cambiato coda
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno.
 */
int log_client_stats(queue_t *client_log, int id, int prod, long time_instore, long time_inqueue, int queuecounter);

/**
 * Alloca la struttura dati del log di una cassa.
 *
 * @param id identificatore univoco della cassa
 * @return il puntatore alla struttura dati allocata oppure NULL in caso di errore ed imposta errno
 */
cassa_log_t *alloc_cassa_log(size_t id);

/**
 * Dsistrugge la struttura dati del log della cassa passata per argomento.
 *
 * @param log puntatore alla struttura dati del log di una cassa
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno
 */
int destroy_cassa_log(cassa_log_t *log);

//TODO finire questa documentazione
int log_client_served(cassa_log_t *cassa_log, long time, int products);
int log_cassa_opening_time(cassa_log_t *cassa_log, long time);
void log_cassa_closed(cassa_log_t *cassa_log);

/**
 * Scrive il file di log. Ritorna -1 se è avvenuto un errore ed imposta errno. In caso di scrittura avvenuta con successo,
 * ritorna 0 ed il file viene chiuso.
 *
 * @param filename nome del file di log
 * @param clients_stats statistiche dei thread clienti
 * @param cassieri_stats statistiche dei cassieri
 * @param k quanti casse ci sono nel supermercato
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno
 */
int write_log(char *filename, queue_t *clients_stats, cassa_log_t **cassieri_stats, int k);

#endif //STATS_H
