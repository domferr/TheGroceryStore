//
// Created by Domenico on 05/07/2020.
//

#ifndef EXECUTORSPOOL_H
#define EXECUTORSPOOL_H

#include "executor.h"
#include <pthread.h>

typedef struct {
    int size;
    executor_t *executors;
} executors_pool_t;

executors_pool_t *newPool(int size);

#endif //EXECUTORSPOOL_H
