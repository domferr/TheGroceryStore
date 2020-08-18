#ifndef STATS_H
#define STATS_H

#include <stdio.h>
#include "queue.h"

/** Statistiche di una cassa del supermercato */
typedef struct {
    size_t id;
    queue_t *served;
    queue_t *opened;
    int closed_counter;
    int products_counter;
} cassa_log_t;

//TODO fare tutto questa documentazione
int log_client_stats(queue_t *client_log, int id, int prod, int time_instore, int time_inqueue, int queuecounter);
cassa_log_t *alloc_cassa_log(size_t id);
int destroy_cassa_log(cassa_log_t *log);
int log_client_served(cassa_log_t *cassa_log, long time, int products);
int log_cassa_opening_time(cassa_log_t *cassa_log, long time);
void log_cassa_closed(cassa_log_t *cassa_log);
int write_log(FILE *out_file, queue_t *clients_stats, cassa_log_t **cassieri_stats, int k);

#endif //STATS_H
