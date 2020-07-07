#include "executor.h"
#include <pthread.h>
#include <stdlib.h>

static void *executorFun(void *arg) {
    executor_t *ex = (executor_t *) arg;
    int running = 1;
    while(running) {
        //TODO waitForJob
        //TODO call the function
    }
    return 0;
}

int executor_create(executor_t *executor) {
    executor = (executor_t*) malloc(sizeof(executor_t));
    if (executor == NULL)
        return -1;

    executor->job = NULL;

    if (pthread_create(&(executor->tid), NULL, executorFun, executor) != 0) {
        free(executor);
        return -1;
    }

    return 0;
}

int execJob(executor_t *executor, job_t *job) {
    //TODO lock
    executor->job = job;
    //TODO unlock
    return 0;
}