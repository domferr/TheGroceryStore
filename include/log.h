#ifndef LOG_H
#define LOG_H

#include "../libs/utils/include/utils.h"
#include "../libs/list/include/list.h"
#include <stdio.h>

/** Statistiche di una cassa del supermercato */
typedef struct {
    size_t id;              //Identificatore univoco della cassa
    list_t *served;         //Tempi di servizio per ogni cliente servito
    list_t *opened;         //Tempi di apertura della cassa per ogni apertura
    int closed_counter;     //Quante volte è stata chiusa la cassa
    int products_counter;   //Quanti prodotti sono stati acquistati tramite questa cassa
    struct timespec open_start; //Istante il cui la cassa è stata aperta l'ultima volta
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
int log_client_stats(list_t *client_log, int id, int prod, long time_instore, long time_inqueue, int queuecounter);

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

/**
 * Aggiorna il log del cassiere con il nuovo cliente servito. Ritorna 0 in caso di successo, -1 altrimenti ed imposta
 * errno.
 *
 * @param cassa_log puntatore alla struttura del log del cassiere
 * @param time quanto tempo è stato necessario per servire il cliente. Espresso in millisecondi
 * @param products quanti prodotti ha acquistato il cliente
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno
 */
int log_client_served(cassa_log_t *cassa_log, long time, int products);

/**
 * Prende il tempo di apertura della cassa. Ritorna 0 in caso di successo, -1 altrimenti ed imposta errno.
 *
 * @param cassa_log puntatore alla struttura del log del cassiere
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno
 */
int log_cassa_open(cassa_log_t *cassa_log);

/**
 * Aggiorna il log del cassiere incrementando il contatore delle chiusure della cassa e calcolando quanto tempo la cassa
 * è stata aperta (se mai è stata aperta).
 *
 * @param cassa_log puntatore alla struttura del log del cassiere
 */
int log_cassa_closed(cassa_log_t *cassa_log);

/**
 * Aggiorna il log nel caso in cui il supermercato viene chiuso. Ritorna 0 in caso di successo, -1 altrimenti ed imposta
 * errno.
 *
 * @param cassa_log puntatore alla struttura del log del cassiere
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno
 */
int log_cassa_store_closed(cassa_log_t *cassa_log);

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
int write_log(char *filename, list_t *clients_stats, cassa_log_t **cassieri_stats, int k);

/**
 * Confronta due statistiche di due clienti. La statistica del cliente con l'identificatore più piccolo è la statistica
 * più "piccola" o che in generale viene prima. Questa funzione ritorna un valore negativo oppure uguale a zero oppure
 * positivo rispettivamente se la prima statistica è minore oppure uguale oppure maggiore della seconda.
 *
 * @param first statistica di un cliente
 * @param second statistica di un altro cliente
 * @return un valore negativo oppure uguale a zero oppure positivo rispettivamente se la prima statistica è minore
 * oppure uguale oppure maggiore della seconda
 */
int compare_client_stats(void *first, void *second);

#endif //LOG_H
