#ifndef CASHIER_H
#define CASHIER_H

#include "grocerystore.h"
#include "client_in_queue.h"
#include "logger.h"
#include "types.h"

/**
 * Funzione svolta dal thread cassiere.
 *
 * @param args un puntatore di tipo cashier_t*
 * @return file di log se il thread è terminato con successo, NULL altrimenti
 */
void *cashier_fun(void *args);

/**
 * Alloca ed imposta una nuova struttura di un cassiere con i parametri passati per argomento.
 * L'identificatore del cassiere viene utilizzato con seed per numeri random, garantendo seed diversi tra tutti i cassieri.
 * Ritorna il puntatore alla struttura dati se la allocazione è avvenuta con successo, NULL altrimenti e setta errno
 *
 * @param id identificatore del cassiere. Utilizzato anche come seed per i numeri random
 * @param gs puntatore alla struttura del supermercato
 * @param starting_state stato iniziale del cassiere, ovvero se la cassa è subito aperta oppure chiusa
 * @param product_service_time quanto tempo impiega questo cassiere a gestire un singolo prodotto
 * @return Ritorna il puntatore alla struttura dati se la allocazione è avvenuta con successo, NULL altrimenti e setta errno
 */
cashier_t *alloc_cashier(size_t id, grocerystore_t *gs, cashier_state starting_state, int product_service_time, int interval, manager_queue *mq);

/**
 * Gestione della chiusura del supermercato da parte di un cassiere
 *
 * @param ca cassiere che deve svolgere le operazioni di terminazione in caso di chiusura supermercato
 * @param closing_state che tipo di chiusura sta avvenendo
 * @param stats puntatore alle statistiche da aggiornare
 * @return 0 se la gestione è avvenuta con successo, -1 altrimenti e setta errno
 */
int handle_closure(cashier_t *ca, gs_state closing_state, cashier_thread_stats *stats);

//TODO update all the entire docs
int serve_client(size_t id, int fixed_time, int product_time, client_in_queue *client, cashier_thread_stats *stats);
int wakeup_client(client_in_queue *client, client_status status);
int notify(cashier_t *ca, int clients_in_queue);
#endif //CASHIER_H
