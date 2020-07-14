#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

typedef struct {
    size_t id;              //id della cassa
    int clients_served;     //numero di clienti serviti
    int **clients_times;    //il tempo di servizio di ogni cliente servito
    int closure_counter;    //numero di chiusure della cassa
    int **opening_times;    //il tempo di ogni periodo di apertura della cassa
    int opening_counter;    //numero di volte che la cassa è stata aperta
} log_cashier;

/**
 * Statistiche di log di ogni thread cliente
 */
typedef struct {
    size_t id;
    int products;       //numero di prodotti acquistati
    int time_in_store;  //tempo di permanenza nel supermercato
    int time_in_queue;  //tempo di attesa in coda
    int queue_counter;  //numero di code visitate
} log_client;

typedef struct {
    int total_products; //numero totale di prodotti acquistati dai clienti che sono entrati con questo thread

} stats_client_thread;

typedef struct {
    size_t id;
    int clients_served;     //numero di clienti serviti
    int closed_counter;     //quante volte la cassa è stata chiusa
    //queue_t active_stats;
} cashier_main_stats;

typedef struct {
    int opening_times;
    //queue_t clients_served;
} cashier_active_stats;

log_client *alloc_client_log(size_t id);
cashier_main_stats *alloc_cashier_stats(size_t id);
int write_log(FILE *out_file, log_client **clients, size_t no_of_clients, log_cashier **cashiers, size_t no_of_cashiers);

#endif //LOGGER_H
