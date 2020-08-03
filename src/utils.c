#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include "../include/utils.h"

#define LONG1E9 1000000000L //1e9

int msleep(int milliseconds) {
    struct timespec req = {MS_TO_SEC(milliseconds), MS_TO_NANOSEC(milliseconds)}, rem = {0, 0};
    int res;
    do {
        rem.tv_sec = 0;
        rem.tv_nsec = 0;
        res = nanosleep(&req, &rem);
        req = rem;
    } while (res == EINTR);

    return res;
}

long get_elapsed_milliseconds(struct timespec *start, struct timespec *end) {
    struct timespec diff = {end->tv_sec - start->tv_sec, end->tv_nsec - start->tv_nsec};
    if (diff.tv_nsec < 0) {
        diff.tv_sec -= 1;
        diff.tv_nsec += LONG1E9;
    }

    return diff.tv_sec * 1000L + diff.tv_nsec / 1000000L;
}

safe_fd_t *get_safe_fd(void) {
    int err;
    safe_fd_t *sfd;
    EQNULL(sfd = (safe_fd_t*) malloc(sizeof(safe_fd_t)), return NULL)
    PTH(err, pthread_mutex_init(&(sfd->mtx), NULL), return NULL)
    return sfd;
}

int free_safe_fd(safe_fd_t *sfd) {
    int err;
    PTH(err, pthread_mutex_destroy(&(sfd->mtx)), return -1)
    free(sfd);
    return 0;
}