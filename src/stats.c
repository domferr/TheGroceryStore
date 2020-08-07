#include <stdlib.h>
#include "../include/stats.h"
#include "../include/utils.h"

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

