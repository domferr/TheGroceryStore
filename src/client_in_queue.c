#include <stdlib.h>
#include "../include/client_in_queue.h"
#include "../libs/utils/include/utils.h"

client_in_queue_t *alloc_client_in_queue(void) {
    int err;
    client_in_queue_t *clq = (client_in_queue_t*) malloc(sizeof(client_in_queue_t));
    EQNULL(clq, return NULL)
    clq->served = 0;
    clq->processing = 0;
    clq->products = 0;
    clq->is_enqueued = 0;
    clq->prev = NULL;
    clq->next = NULL;
    PTH(err, pthread_cond_init(&(clq->waiting), NULL), return NULL)

    return clq;
}

int destroy_client_in_queue(client_in_queue_t *clq) {
    int err;
    PTH(err, pthread_cond_destroy(&(clq->waiting)), return -1)
    free(clq);
    return 0;
}

int wakeup_client(client_in_queue_t *clq, int served) {
    int err;
    clq->served = served;
    clq->processing = 0;
    PTH(err, pthread_cond_signal(&(clq->waiting)), return -1)
    return 0;
}