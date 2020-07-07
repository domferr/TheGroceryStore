//
// Created by Domenico on 07/07/2020.
//

#include "executor.h"
#include <pthread.h>
#include <stdlib.h>

static void waitForJob(executor_t *ex) {
    pthread_mutex_lock(&(ex->mtx));
    while (!(ex->hasJob)) {
        pthread_cond_wait(&(ex->noJob), &(ex->mtx));
    }
    pthread_mutex_unlock(&(ex->mtx));
}

static void *executorFun(void *arg) {
    executor_t *ex = (executor_t *) arg;
    int running = 1;
    while(running) {
        waitForJob(ex);
        ex->jobFun(ex->funArgs);
    }
}

int executor_create(executor_t *executor) {
    executor = (executor_t*) malloc(sizeof(executor_t));
    if (executor == NULL)
        return -1;

    executor->hasJob = 0;
    executor->jobFun = NULL;
    executor->funArgs = NULL;
    if (pthread_mutex_init(&(executor->mtx), NULL) != 0) {
        free(executor);
        return -1;
    }

    if (pthread_cond_init(&(executor->noJob), NULL) != 0) {
        pthread_mutex_destroy(&(executor->mtx));
        free(executor);
        return -1;
    }

    if (pthread_create(&(executor->tid), NULL, executorFun, executor) != 0) {
        pthread_cond_destroy(&(executor->noJob));
        pthread_mutex_destroy(&(executor->mtx));
        free(executor);
        return -1;
    }

    return 0;
}

int execJob(executor_t *executor, void *(*jobFun)(void *), void *funArgs) {
    pthread_mutex_lock(&(executor->mtx));
    executor->hasJob = 1;
    executor->jobFun = jobFun;
    executor->funArgs = funArgs;
    pthread_mutex_unlock(&(executor->mtx));
    return 0;
}