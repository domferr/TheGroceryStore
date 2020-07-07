//
// Created by Domenico on 05/07/2020.
//

#include "executorspool.h"
#include "executor.h"
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

executors_pool_t *newPool(int size) {
    int i;
    executors_pool_t *pool = (executors_pool_t*) malloc(sizeof(executors_pool_t));
    if (pool == NULL) return NULL;
    pool->size = size;
    pool->executors = (executor_t*) malloc(sizeof(executor_t)*size);
    if (pool->executors == NULL) {
        free(pool);
        return NULL;
    }
    //creo size threads
    for (i=0; i<size; i++) {
        if (executor_create(&(pool->executors[i])) != 0)
            perror("executor_init");
        //TODO gestione fallimento newExecutor
    }
    return pool;
}