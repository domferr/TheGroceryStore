#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

/** Statistiche raccolte da un thread cliente */
typedef struct {
    size_t id;          //identificatore del thread cliente

} client_thread_stats;

/** Statistiche raccolte da un thread cassiere */
typedef struct {
    size_t id;              //identificatore del thread cassiere
    int clients_served;     //numero di clienti serviti
    int closed_counter;     //quante volte la cassa è stata chiusa
    int total_products;     //numero prodotti acquistati tramite questa cassa
} cashier_thread_stats;



typedef struct {
    size_t id;              //id della cassa
    int clients_served;     //numero di clienti serviti
    int **clients_times;    //il tempo di servizio di ogni cliente servito
    int closure_counter;    //numero di chiusure della cassa
    int **opening_times;    //il tempo di ogni periodo di apertura della cassa
    int opening_counter;    //numero di volte che la cassa è stata aperta
} log_cashier;

typedef struct {
    size_t id;
    int products;       //numero di prodotti acquistati
    int time_in_store;  //tempo di permanenza nel supermercato
    int time_in_queue;  //tempo di attesa in coda
    int queue_counter;  //numero di code visitate
} log_client;


client_thread_stats *alloc_client_thread_stats(size_t id);
void destroy_client_thread_stats(client_thread_stats *stats);
cashier_thread_stats *alloc_cashier_thread_stats(size_t id);
void destroy_cashier_thread_stats(cashier_thread_stats *stats);
int write_log(FILE *out_file, client_thread_stats **clients, size_t no_of_clients, cashier_thread_stats **cashiers, size_t no_of_cashiers);

#endif //LOGGER_H
