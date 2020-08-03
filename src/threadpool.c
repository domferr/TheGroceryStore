#include "../include/threadpool.h"
#include "../include/utils.h"
#include <stdlib.h>
#include <unistd.h>

thread_pool_t *thread_pool_create(int max_size) {
    int err;
    thread_pool_t *pool = (thread_pool_t*) malloc(sizeof(thread_pool_t));
    EQNULL(pool, return NULL)

    pool->max_size = max_size;
    pool->size = 0;
    EQNULL(pool->threads = (pthread_t*) malloc(sizeof(pthread_t)*max_size), free(pool); return NULL)
    EQNULL(pool->args = (void*) malloc(sizeof(void*)*max_size), return NULL)
    EQNULL(pool->retvalues = (void*) malloc(sizeof(void*)*max_size), return NULL)

    PTH(err, pthread_mutex_init(&(pool->mtx), NULL), free(pool->threads); free(pool); return NULL)

    return pool;
}

int thread_pool_join(thread_pool_t *pool) {
    int err;
    size_t i;
    void *thread_return;

    PTH(err, pthread_mutex_lock(&(pool->mtx)), return -1)
    for (i = 0; i < pool->size; ++i) {
        PTH(err, pthread_join(pool->threads[i], &thread_return), return -1)
        pool->retvalues[i] = thread_return;
    }
    PTH(err, pthread_mutex_unlock(&(pool->mtx)), return -1)
    return 0;
}

int thread_pool_free(thread_pool_t *pool) {
    int err;
    free(pool->threads);
    free(pool->args);
    free(pool->retvalues);
    PTH(err, pthread_mutex_destroy(&(pool->mtx)), return -1)
    free(pool);
    return 0;
}

int thread_create(thread_pool_t *pool, void *(*fun)(void *), void *args) {
    int err;
    PTH(err, pthread_mutex_lock(&(pool->mtx)), return -1)
    if (pool->size < pool->max_size) {
        pool->args[pool->size] = args;
        PTH(err, pthread_create(&(pool->threads[pool->size]), NULL, fun, args), return -1)
        (pool->size)++;
    }
    PTH(err, pthread_mutex_unlock(&(pool->mtx)), return -1)
    return 0;
}