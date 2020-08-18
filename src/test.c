#include <stdio.h>
#include "../include/client_in_queue.h"
#include "../include/cassa_queue.h"

static int print(void *elem, void *args) {
    client_in_queue_t *clq = (client_in_queue_t*) elem;
    printf("%d\n", clq->products);
    return 0;
}

int main(int argc, char **args) {
    cassa_queue_t *queue = cassa_queue_create();
    client_in_queue_t *clq1 = alloc_client_in_queue();
    cassiere_t cassiere = {};
    cassiere.queue = queue;
    clq1->products = 1;
    client_in_queue_t *clq2 = alloc_client_in_queue();
    clq2->products = 2;
    client_in_queue_t *clq3 = alloc_client_in_queue();
    clq3->products = 3;
    enqueue(&cassiere, clq1);
    enqueue(&cassiere, clq2);
    enqueue(&cassiere, clq3);

    dequeue(&cassiere, clq1);
    destroy_client_in_queue(clq1);
    printf("%d\n", clq1->is_enqueued);
    client_in_queue_t *next = get_next_client(&cassiere);
    printf("prod: %d\n", next->products);
    next = get_next_client(&cassiere);
    printf("prod: %d\n", next->products);
    printf("queue size: %d\n", queue->size);
    destroy_client_in_queue(clq2);
    destroy_client_in_queue(clq3);
    cassa_queue_destroy(queue);
    return 0;
}