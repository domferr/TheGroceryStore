#include "logger.h"
#include "utils.h"
#include <stdlib.h>

log_client *alloc_client_log(size_t id) {
    log_client *log = (log_client*) malloc(sizeof(log_client));
    EQNULL(log, return NULL);
    log->id = id;
    log->products = 0;       //numero di prodotti acquistati
    log->time_in_store = 0;  //tempo di permanenza nel supermercato
    log->time_in_queue = 0;  //tempo di attesa in coda
    log->queue_counter = 0;  //numero di code visitate
    return log;
}

log_cashier *alloc_cashier_log(size_t id) {
    log_cashier *log = (log_cashier*) malloc(sizeof(log_cashier));
    EQNULL(log, return NULL);
    log->id = id;
    log->clients_served = 0;     //numero di clienti serviti
    log->closure_counter = 0;    //numero di prodotti acquistati
    return log;
}

int write_log(FILE *out_file, log_client **clients, size_t no_of_clients, log_cashier **cashiers, size_t no_of_cashiers) {
    int total_clients, total_products;  //il numerodi clienti serviti, il numero di prodotti acquistati

    return 0;
}