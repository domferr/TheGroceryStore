#ifndef CASSA_QUEUE_H
#define CASSA_QUEUE_H
#include "storetypes.h"

cassa_queue_t *cassa_queue_create(void);
int cassa_queue_destroy(cassa_queue_t *q);
int get_queue_cost(cassiere_t *cassiere, client_in_queue_t *clq);
client_in_queue_t *get_next_client(cassiere_t *cassiere, int processing);
int enqueue(cassiere_t *cassiere, client_in_queue_t *clq);
void dequeue(cassiere_t *cassiere, client_in_queue_t *clq);
cassiere_t *get_best_queue(cassiere_t **cassieri, int k, cassiere_t *from, int from_cost);
int join_queue(cassiere_t *cassiere, client_in_queue_t *clq, struct timespec *queue_entrance);

#endif //CASSA_QUEUE_H
