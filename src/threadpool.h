#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "executor.h"
#include <pthread.h>

typedef struct {
    size_t max_size;    //Al massimo quanti elementi il pool può contenere
    size_t size;    //quanti elementi ci sono nel pool
    pthread_t *threads; //array di threads
    pthread_mutex_t mtx;
} thread_pool_t;

thread_pool_t *thread_pool_create(int max_size);
void thread_pool_join(thread_pool_t *pool);
void thread_pool_free(thread_pool_t *pool);
int thread_create(thread_pool_t *pool, void *(*fun)(void *), void *args);

#endif //THREADPOOL_H