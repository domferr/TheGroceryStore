#include "utils.h"
#include "client_in_queue.h"
#include "cashier.h"
#include <stdio.h>
#include <stdlib.h>

int serve_client(cashier_t *ca, client_in_queue *client) {
    int err, ms;
    PTH(err, pthread_mutex_lock(&(client->mutex)), return -1)
    ms = ca->fixed_service_time + (ca->product_service_time)*client->products;
    PTH(err, pthread_mutex_unlock(&(client->mutex)), return -1)
#ifdef DEBUG_CASHIER
    printf("Cassiere %ld: Servo un cliente. Impiegherò %dms\n", ca->id, ms);
#endif
    MINUS1(msleep(ms), return -1);
    return wakeup_client(client, done);
}

int wakeup_client(client_in_queue *client, client_status status) {
    int err;
    PTH(err, pthread_mutex_lock(&(client->mutex)), return -1)
    client->status = status;
    PTH(err, pthread_cond_signal(&(client->waiting)), return -1)
    PTH(err, pthread_mutex_unlock(&(client->mutex)), return -1)
    return 0;
}