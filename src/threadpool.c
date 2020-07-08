#include "threadpool.h"
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

thread_pool_t *thread_pool_create(int max_size) {
    thread_pool_t *pool = (thread_pool_t*) malloc(sizeof(thread_pool_t));
    if (pool == NULL) return NULL;

    pool->max_size = max_size;
    pool->size = 0;
    pool->threads = (pthread_t*) malloc(sizeof(pthread_t)*max_size);
    if (pool->threads == NULL) {
        free(pool);
        return NULL;
    }

    if (pthread_mutex_init(&(pool->mtx), NULL) != 0) {
        free(pool->threads);
        free(pool);
        return NULL;
    }

    return pool;
}

void thread_pool_join(thread_pool_t *pool) {
    size_t i = 0;
    void *retval;
    pthread_mutex_lock(&(pool->mtx));
    for (i = 0; i < pool->size; ++i) {
        pthread_join(pool->threads[i], &retval);
    }
    pthread_mutex_unlock(&(pool->mtx));
}

void thread_pool_free(thread_pool_t *pool) {
    free(pool->threads);
    pthread_mutex_destroy(&(pool->mtx));
    free(pool);
}

int thread_create(thread_pool_t *pool, void *(*fun)(void *), void *args) {
    pthread_mutex_lock(&(pool->mtx));
    if (pool->size < pool->max_size) {
        if (pthread_create(&(pool->threads[pool->size]), NULL, fun, args) != 0) {
            pthread_mutex_unlock(&(pool->mtx));
            return -1;
        }
        (pool->size)++;
    }
    pthread_mutex_unlock(&(pool->mtx));
    return 0;
}