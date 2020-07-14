#include "logger.h"
#include "utils.h"
#include <stdlib.h>

client_thread_stats *alloc_client_thread_stats(size_t id) {
    client_thread_stats *stats = (client_thread_stats*) malloc(sizeof(client_thread_stats));
    EQNULL(stats, return NULL);
    stats->id = id; //identificatore del thread cliente
    stats->total_products = 0;
    return stats;
}

void destroy_client_thread_stats(client_thread_stats *stats) {
    free(stats);
}

cashier_thread_stats *alloc_cashier_thread_stats(size_t id) {
    cashier_thread_stats *stats = (cashier_thread_stats*) malloc(sizeof(cashier_thread_stats));
    EQNULL(stats, return NULL);
    stats->id = id;
    stats->clients_served = 0;  //numero di clienti serviti
    stats->closed_counter = 0;  //numero di volte che la cassa è stata chiusa
    return stats;
}

void destroy_cashier_thread_stats(cashier_thread_stats *stats) {
    free(stats);
}

int write_log(FILE *out_file, client_thread_stats **clients, size_t no_of_clients, cashier_thread_stats **cashiers, size_t no_of_cashiers) {
    int total_clients, total_products;  //il numerodi clienti serviti, il numero di prodotti acquistati

    return 0;
}