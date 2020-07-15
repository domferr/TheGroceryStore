#ifndef MANAGER_STRUCT_H
#define MANAGER_STRUCT_H

#include "cashier.h"

typedef struct {
    size_t size;
    cashier_sync **ca_sync;
    int **counters;
} manager_arr_t;

#endif //MANAGER_STRUCT_H
