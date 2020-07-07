#include "executorspool.h"
#include "executor.h"
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

executors_pool_t *executors_pool_create(int size) {
    int i = 0;
    executors_pool_t *pool = (executors_pool_t*) malloc(sizeof(executors_pool_t));
    if (pool == NULL) return NULL;
    pool->size = size;
    pool->executors = (executor_t**) malloc(sizeof(executor_t*)*size);
    if (pool->executors == NULL) {
        free(pool);
        return NULL;
    }
    pool->jobs = (job_t*) malloc(sizeof(job_t)*size);
    if (pool->jobs == NULL) {
        free(pool->executors);
        free(pool);
        return NULL;
    }
    if (pthread_mutex_init(&(pool->mtx), NULL) != 0) {
        free(pool->jobs);
        free(pool->executors);
        free(pool);
        return NULL;
    }
    //creo size threads
    for (i=0; i<size; i++) {
        if (executor_create(pool->executors[i]) != 0) {
            pthread_mutex_destroy(&(pool->mtx));
            free(pool->jobs);
            free(pool->executors);
            free(pool);
            return NULL;
            //TODO gestione fallimento executor_create
        }
    }
    return pool;
}

int exec(executors_pool_t *pool, void *(*jobFun)(void *), void *funArgs) {
    size_t i = 0;
    job_t *job = (job_t*) malloc(sizeof(job_t));
    if (job == NULL) {
        return -1;
    }
    job->jobFun = jobFun;
    job->funArgs = funArgs;

    //TODO eseguire il lavoro
    return 0;
}