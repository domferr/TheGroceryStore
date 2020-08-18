#include <stdlib.h>
#include <stdio.h>
#include "../include/stats.h"
#include "../include/utils.h"

static int write_client_stats(void *elem, void *args);
static int print_ms(void *elem, void *args);

/** Statistiche di un cliente entrato nel supermercato */
typedef struct {
    int id;             //identificatore univoco del cliente
    int products;       //numero di prodotti acquistati
    long time_in_store; //tempo di permanenza nel supermercato espresso in millisecondi
    long time_in_queue; //tempo di attesa in coda espresso in millisecondi
    int queue_counter;  //numero di code visitate
} client_stats_t;

//list -> cliente servito (e quindi size=tot clients serviti)
//list -> tempo apertura
//int chiusure

int log_client_stats(queue_t *client_log, int id, int prod, int time_instore, int time_inqueue, int queuecounter) {
    client_stats_t *stats = (client_stats_t*) malloc(sizeof(client_stats_t));
    EQNULL(stats, return -1)
    stats->id = id;
    stats->products = prod;
    stats->time_in_store = time_instore;
    stats->time_in_queue = time_inqueue;
    stats->queue_counter = queuecounter;

    MINUS1(push(client_log, stats), return -1)
    return 0;
}

cassa_log_t *alloc_cassa_log(size_t id) {
    cassa_log_t *log = (cassa_log_t*) malloc(sizeof(cassa_log_t));
    EQNULL(log, return NULL)
    log->id = id;
    log->closed_counter = 0;
    log->products_counter = 0;
    EQNULL(log->served = queue_create(), return NULL)
    EQNULL(log->opened = queue_create(), return NULL)
    return log;
}

int destroy_cassa_log(cassa_log_t *log) {
    MINUS1(queue_destroy(log->served, &free), return -1)
    MINUS1(queue_destroy(log->opened, &free), return -1)
    free(log);
    return 0;
}

int log_client_served(cassa_log_t *cassa_log, long time, int products) {
    long *value = (long*) malloc(sizeof(long));
    *value = time;
    EQNULL(value, return -1)
    MINUS1(push(cassa_log->served, value), return -1)
    cassa_log->products_counter += products;
    return 0;
}

int log_cassa_opening_time(cassa_log_t *cassa_log, long time) {
    long *value = (long*) malloc(sizeof(long));
    *value = time;
    EQNULL(value, return -1)
    MINUS1(push(cassa_log->opened, value), return -1)
    return 0;
}

void log_cassa_closed(cassa_log_t *cassa_log) {
    cassa_log->closed_counter++;
}

int write_log(FILE *out_file, queue_t *clients_stats, cassa_log_t **cassieri_stats, int k) {
    int i, tot_clients = 0, total_products = 0;  //il numero di clienti serviti, il numero di prodotti acquistati
    queue_t *served, *opened;
    //Scrivo log accumulato dai clienti
    MINUS1(foreach(clients_stats, &write_client_stats, out_file), return -1)

    //TODO Scrivo log accumulato dai cassieri
    for (i = 0; i < k; i++) {
        served = cassieri_stats[i]->served;
        opened = cassieri_stats[i]->opened;

        fprintf(out_file, "[Cassiere %ld] clienti serviti: %d, prodotti elaborati: %d, chiusure: %d",
                (cassieri_stats[i])->id, served->size, cassieri_stats[i]->products_counter, cassieri_stats[i]->closed_counter);
        if (opened->size > 0) { //Se la cassa è stata aperta almeno una volta
            fprintf(out_file, ", periodi di apertura cassa:");
            foreach(opened, &print_ms, out_file);
        }
        if (served->size > 0) { //Se è stato servito almeno un cliente
            fprintf(out_file, ", tempi di servizio dei clienti serviti:");
            foreach(served, &print_ms, out_file);
        }
        fprintf(out_file, "\n");

        total_products += cassieri_stats[i]->products_counter;
        tot_clients += served->size;
    }

    //Scrivo le statistiche generali
    fprintf(out_file, "Numero di clienti serviti: %d\n", clients_stats->size);
    fprintf(out_file, "Numero di prodotti acquistati: %d\n", total_products);
    if (tot_clients != clients_stats->size)
        printf("------------------- NON COMBACIANOOOOOOOOOOOOOOOOOOO. Casse: %d\t Clienti: %d\n", tot_clients, clients_stats->size);
    return 0;
}

static int write_client_stats(void *elem, void *args) {
    client_stats_t *cl_stats = (client_stats_t*) elem;
    fprintf((FILE*)args, "[Cliente %d] tempo di permanenza: %ldms, prodotti acquistati: %d, code cambiate: %d, tempo in coda: %ldms\n", cl_stats->id, cl_stats->time_in_store, cl_stats->products, cl_stats->queue_counter, cl_stats->time_in_queue);
    return 0;
}

static int print_ms(void *elem, void *args) {
    long *value = (long*) elem;
    fprintf((FILE*)args, " %ldms", *value);
    return 0;
}