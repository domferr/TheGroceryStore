#include "logger.h"
#include "utils.h"
#include "queue.h"
#include <stdlib.h>

static int write_client_stats(void *elem, void *args);
static int print_int_ms(void *elem, void *args);

client_thread_stats *alloc_client_thread_stats(size_t id) {
    client_thread_stats *stats = (client_thread_stats*) malloc(sizeof(client_thread_stats));
    EQNULL(stats, return NULL)
    stats->id = id; //identificatore del thread cliente
    stats->queue = queue_create();
    EQNULL(stats->queue, return NULL)

    return stats;
}

client_stats *alloc_client_stats(size_t id) {
    client_stats *stats = (client_stats*) malloc(sizeof(client_stats));
    EQNULL(stats, return NULL)

    stats->id = id;
    stats->products = 0;
    stats->time_in_store = 0;
    stats->time_in_queue = 0;
    stats->queue_counter = 0;

    return stats;
}

void destroy_client_thread_stats(client_thread_stats *stats) {
    queue_destroy(stats->queue);
    free(stats);
}

cashier_thread_stats *alloc_cashier_thread_stats(size_t id) {
    cashier_thread_stats *stats = (cashier_thread_stats*) malloc(sizeof(cashier_thread_stats));
    EQNULL(stats, return NULL);
    stats->id = id;
    stats->closed_counter = 0;  //numero di volte che la cassa è stata chiusa
    stats->total_products = 0;  //numero totale di prodotti acquistati tramite questa cassa

    stats->active_times = queue_create();   //tempi di apertura della cassa
    EQNULL(stats->active_times, return NULL)

    stats->clients_times = queue_create();  //tempo di servizio di ogni cliente servito. La dimensione indica numero di clienti serviti
    EQNULL(stats->clients_times, return NULL)

    return stats;
}

void destroy_cashier_thread_stats(cashier_thread_stats *stats) {
    queue_destroy(stats->active_times);
    queue_destroy(stats->clients_times);
    free(stats);
}

int write_log(FILE *out_file, client_thread_stats **clients, size_t no_of_clients, cashier_thread_stats **cashiers, size_t no_of_cashiers) {
    int total_clients = 0, total_products = 0, clients_served = 0;  //il numero di clienti serviti, il numero di prodotti acquistati
    size_t i;
    queue_t *queue;

    //Scrivo log accumulato dai clienti
    for (i = 0; i < no_of_clients; ++i) {
        EQNULL((clients[i]), continue);
        queue = (clients[i])->queue;
        foreach(queue, &write_client_stats, out_file);
    }

    //Scrivo log accumulato dai cassieri
    for (i = 0; i < no_of_cashiers; i++) {
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
    }
    fprintf(out_file, "Numero di clienti serviti: %d\n", total_clients);
    fprintf(out_file, "Numero di prodotti acquistati: %d\n", total_products);
    return 0;
}

static int write_client_stats(void *elem, void *args) {
    client_stats *cl_stats = (client_stats*) elem;
    fprintf((FILE*)args, "[Cliente %ld] tempo di permanenza: %ldms, prodotti acquistati: %d, code cambiate: %d, tempo in coda: %ldms\n", cl_stats->id, cl_stats->time_in_store, cl_stats->products, cl_stats->queue_counter, cl_stats->time_in_queue);
    return 0;
}

static int print_int_ms(void *elem, void *args) {
    int *value = (int*) elem;
    fprintf((FILE*)args, " %dms", *value);
    return 0;
}