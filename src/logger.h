#ifndef LOGGER_H
#define LOGGER_H

#include "queue.h"
#include <stdio.h>

/** Statistiche raccolte da un thread cliente */
typedef struct {
    size_t id;          //identificatore del thread cliente
    queue_t *queue;     //statistiche per ogni cliente che è entrato nel supermercato mediante questo thread
} client_thread_stats;

/** Statistiche di un cliente entrato nel supermercato */
typedef struct {
    size_t id;          //identificatore univoco del cliente
    int products;       //numero di prodotti acquistati
    long time_in_store; //tempo di permanenza nel supermercato espresso in millisecondi
    long time_in_queue; //tempo di attesa in coda espresso in millisecondi
    int queue_counter;  //numero di code visitate
} client_stats;

/** Statistiche raccolte da un thread cassiere */
typedef struct {
    size_t id;              //identificatore del thread cassiere
    int closed_counter;     //quante volte la cassa è stata chiusa
    int total_products;     //numero prodotti acquistati tramite questa cassa
    queue_t *active_times;  //tempi di apertura della cassa
    queue_t *clients_times; //tempo di servizio di ogni cliente servito. La sua dimensione indica anche il numero di clienti serviti
} cashier_thread_stats;

client_thread_stats *alloc_client_thread_stats(size_t id);

client_stats *alloc_client_stats(size_t id);

cashier_thread_stats *alloc_cashier_thread_stats(size_t id);

void destroy_client_thread_stats(client_thread_stats *stats);

void destroy_cashier_thread_stats(cashier_thread_stats *stats);

int write_log(FILE *out_file, client_thread_stats **clients, size_t no_of_clients, cashier_thread_stats **cashiers, size_t no_of_cashiers);

#endif //LOGGER_H
