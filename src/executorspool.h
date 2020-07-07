#ifndef EXECUTORSPOOL_H
#define EXECUTORSPOOL_H

#include "executor.h"
#include <pthread.h>

typedef struct {
    size_t size;
    executor_t **executors;
    size_t howManyJobs;
    job_t *jobs;    //Da sostituire con una unbounded queue
    pthread_mutex_t mtx;
} executors_pool_t;

executors_pool_t *executors_pool_create(int size);
int exec(executors_pool_t *pool, void *(*jobFun)(void *), void *funArgs);

#endif //EXECUTORSPOOL_H