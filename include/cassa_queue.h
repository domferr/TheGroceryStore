#ifndef CASSA_QUEUE_H
#define CASSA_QUEUE_H
#include "storetypes.h"

typedef struct {
    pthread_cond_t noclients;   //il cassiere aspetta su questa variabile di condizione quando non ci sono clienti
    client_in_queue_t *first;
    client_in_queue_t *last;
    int size;
    int totprod;
} cassa_queue_t;

cassa_queue_t *cassa_queue_create(void);
int cassa_queue_destroy(cassa_queue_t *q);
int join_queue(cassa_queue_t *q, client_in_queue_t *clq);
int leave_queue(cassa_queue_t *q, client_in_queue_t *clq);

#endif //CASSA_QUEUE_H
