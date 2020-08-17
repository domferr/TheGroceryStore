#include <stdlib.h>
#include <stdio.h>
#include "../include/stats.h"
#include "../include/utils.h"

static int write_client_stats(void *elem, void *args);

client_stats *new_client_stats(int id) {
    client_stats *stats = (client_stats*) malloc(sizeof(client_stats));
    EQNULL(stats, return NULL)

    stats->id = id;
    stats->products = 0;
    stats->time_in_store = 0;
    stats->time_in_queue = 0;
    stats->queue_counter = 0;

    return stats;
}

int write_log(FILE *out_file, queue_t *clients_stats) {
    int total_products = 0;  //il numero di clienti serviti, il numero di prodotti acquistati

    //Scrivo log accumulato dai clienti
    MINUS1(foreach(clients_stats, &write_client_stats, out_file), return -1)

    //TODO Scrivo log accumulato dai cassieri
    /*for (i = 0; i < no_of_cashiers; i++) {
        EQNULL((cashiers[i]), continue);
        clients_served = ((cashiers[i])->clients_times)->size;
        fprintf(out_file, "[Cassiere %ld] clienti serviti: %d, prodotti elaborati: %d, chiusure: %d", (cashiers[i])->id, clients_served, (cashiers[i])->total_products, (cashiers[i])->closed_counter);
        queue = (cashiers[i])->active_times;
        if (queue->size > 0) {
            fprintf(out_file, ", periodi di apertura cassa:");
            foreach(queue, &print_int_ms, out_file);
        }
        queue = (cashiers[i])->clients_times;
        if (queue->size > 0) {
            fprintf(out_file, ", tempi di servizio dei clienti serviti:");
            foreach(queue, &print_int_ms, out_file);
        }
        fprintf(out_file, "\n");

        total_products += (cashiers[i])->total_products;
        total_clients += clients_served;
    }*/

    //Scrivo le statistiche generali
    fprintf(out_file, "Numero di clienti serviti: %d\n", clients_stats->size);
    fprintf(out_file, "Numero di prodotti acquistati: %d\n", total_products);
    return 0;
}

static int write_client_stats(void *elem, void *args) {
    client_stats *cl_stats = (client_stats*) elem;
    fprintf((FILE*)args, "[Cliente %d] tempo di permanenza: %ldms, prodotti acquistati: %d, code cambiate: %d, tempo in coda: %ldms\n", cl_stats->id, cl_stats->time_in_store, cl_stats->products, cl_stats->queue_counter, cl_stats->time_in_queue);
    return 0;
}
/*
static int print_int_ms(void *elem, void *args) {
    int *value = (int*) elem;
    fprintf((FILE*)args, " %dms", *value);
    return 0;
}*/