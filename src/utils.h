#ifndef UTILS_H
#define UTILS_H

#include <errno.h>

#define EQNULL(w, then)     \
        if (w == NULL) {    \
            then;           \
        }
#define MINUS1(w, then) \
        if (w == -1) {  \
            then;       \
        }
#define ZERO(w, then)   \
        if (w == 0) {   \
            then;       \
        }
#define NOTZERO(w, then)    \
        if (w != 0) {       \
            then;           \
        }
#define PTH(e, pcall, then)         \
        if ((e = pcall) != 0) {     \
            errno = e;              \
            perror(#e);             \
            then;                    \
        }

#define MS_TO_SEC(ms) ms/1000
#define MS_TO_NANOSEC(ms) ms%1000000
#define RANDOM(seed, min, max) (rand_r(&seed)%(max-min)) + min

int msleep(int milliseconds);

#endif //UTILS_H
