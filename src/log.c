#include "../include/utils.h"
#include "../include/log.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * Scrive le statistiche di un cliente entrato nel supermercato. Ritorna 0 se la scrittura è avvenuta con successo, -1
 * altrimenti e setta errno. Questa funzione è eseguita per ogni elemento della lista quindi il valore di ritorno è
 * importante per evitare errori in cascata.
 *
 * @param elem elemento della lista, ovvero la statistica di un cliente
 * @param args argomenti aggiuntivi, ovvero il file sul quale scrivere il log
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno
 */
static int write_client_stats(void *elem, void *args);

/**
 * Scrive un long sul file descriptor passato. Il valore viene scritto seguito dalla stringa "ms". Questa funzione è
 * eseguita per ogni elemento della lista quindi il valore di ritorno è importante per evitare errori in cascata.
 *
 * @param elem elemento della lista, ovvero un valore di tipo long che rappresenta un tempo espresso in millisecondi
 * @param args argomenti aggiuntivi, ovvero il file sul quale scrivere il log
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno
 */
static int print_ms(void *elem, void *args);

/** Statistiche di un cliente entrato nel supermercato */
typedef struct {
    int id;             //identificatore univoco del cliente
    int products;       //numero di prodotti acquistati
    long time_in_store; //tempo di permanenza nel supermercato espresso in millisecondi
    long time_in_queue; //tempo di attesa in coda espresso in millisecondi
    int queue_counter;  //quante volte il cliente ha cambiato coda
} client_stats_t;

int log_client_stats(queue_t *client_log, int id, int prod, long time_instore, long time_inqueue, int queuecounter) {
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

int write_log(char *filename, queue_t *clients_stats, cassa_log_t **cassieri_stats, int k) {
    FILE *out_file = fopen(filename, "w");
    EQNULL(out_file, return -1);
    int i, served_clients = 0, total_products = 0, noproducts;  //il numero di clienti serviti, il numero di prodotti acquistati
    queue_t *served, *opened;
    //Scrivo log accumulato dai clienti
    MINUS1(foreach(clients_stats, &write_client_stats, out_file), return -1)
    //Scrivo log accumulato dai cassieri
    for (i = 0; i < k; i++) {
        served = cassieri_stats[i]->served;
        opened = cassieri_stats[i]->opened;

        fprintf(out_file, "[Cassiere %ld] clienti serviti: %d, prodotti elaborati: %d, chiusure: %d",
                (cassieri_stats[i])->id, served->size, cassieri_stats[i]->products_counter, cassieri_stats[i]->closed_counter);
        if (opened->size > 0) { //Se la cassa è stata aperta almeno una volta
            fprintf(out_file, ", periodi di apertura cassa:");
            MINUS1(foreach(opened, &print_ms, out_file), return -1)
        }
        if (served->size > 0) { //Se è stato servito almeno un cliente
            fprintf(out_file, ", tempi di servizio dei clienti serviti:");
            MINUS1(foreach(served, &print_ms, out_file), return -1)
        }
        fprintf(out_file, "\n");

        total_products += cassieri_stats[i]->products_counter;
        served_clients += served->size;
    }
    //Clienti usciti senza fare acquisti = clienti entrati - clienti serviti
    noproducts = clients_stats->size - served_clients;

    //Scrivo le statistiche generali
    fprintf(out_file, "Numero di clienti entrati nel supermercato: %d\n", clients_stats->size);
    fprintf(out_file, "Numero di clienti usciti senza acquisti: %d\n", noproducts);
    fprintf(out_file, "Numero di clienti serviti: %d\n", served_clients);
    fprintf(out_file, "Numero di prodotti acquistati: %d\n", total_products);
    fflush(out_file);
    fclose(out_file);
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