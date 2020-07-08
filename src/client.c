#include "client.h"
#include <stdio.h>
#include <pthread.h>

void *client(void *args) {
    printf("Cliente attivo\n");

    return 0;
}