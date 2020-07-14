#include "logger.h"
#include "utils.h"
#include <stdlib.h>

client_thread_stats *alloc_client_thread_stats(size_t id) {
    client_thread_stats *stats = (client_thread_stats*) malloc(sizeof(client_thread_stats));
    EQNULL(stats, return NULL);
    stats->id = id; //identificatore del thread cliente

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
    stats->total_products = 0;
    return stats;
}

void destroy_cashier_thread_stats(cashier_thread_stats *stats) {
    free(stats);
}

int write_log(FILE *out_file, client_thread_stats **clients, size_t no_of_clients, cashier_thread_stats **cashiers, size_t no_of_cashiers) {
    int total_clients = 0, total_products = 0;  //il numero di clienti serviti, il numero di prodotti acquistati
    size_t i;
    for (i = 0; i < no_of_clients; ++i) {
        printf("Cliente %ld: ancora nessuna statistica\n", (clients[i])->id);
    }
    for (i = 0; i < no_of_cashiers; i++) {
        printf("Cassiere %ld: clienti serviti: %d; prodotti elaborati: %d; chiusure %d;\n", (cashiers[i])->id, (cashiers[i])->clients_served, (cashiers[i])->total_products, (cashiers[i])->closed_counter);
        total_products += (cashiers[i])->total_products;
        total_clients += (cashiers[i])->clients_served;
    }
    printf("Numero di clienti serviti: %d\n", total_clients);
    printf("Numero di prodotti acquistati: %d\n", total_products);
    return 0;
}