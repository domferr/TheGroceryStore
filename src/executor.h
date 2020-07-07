#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <pthread.h>

typedef struct {
    void *(*jobFun)(void*);
    void *funArgs;
} job_t;

typedef struct {
    pthread_t tid;
    job_t *job;
} executor_t;

int executor_create(executor_t *executor);
int execJob(executor_t *executor, job_t *job);

#endif //EXECUTOR_H
