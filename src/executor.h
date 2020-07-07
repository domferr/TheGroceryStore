//
// Created by Domenico on 07/07/2020.
//

#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <pthread.h>

typedef struct {
    pthread_t tid;
    pthread_mutex_t mtx;
    pthread_cond_t noJob;
    int hasJob;
    void *(*jobFun)(void*);
    void *funArgs;
} executor_t;

int executor_create(executor_t *executor);
int execJob(executor_t *executor, void *(*jobFun)(void *), void *funArgs);

#endif //EXECUTOR_H
