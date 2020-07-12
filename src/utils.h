#ifndef UTILS_H
#define UTILS_H

#define _POSIX_C_SOURCE 200809L

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
            then;                   \
        }

/**
 * Conversione da millisecondi a secondi
 */
#define MS_TO_SEC(ms) ms/1000

/**
 * Quanti nanosecondi ci sono nei millisecondi specificati
 */
#define MS_TO_NANOSEC(ms) ms%1000000

/**
 * Genera numero random tra min (incluso) e max (non incluso)
 */
#define RANDOM(seed, min, max) (rand_r(&seed)%(max-min)) + min

/**
 * Mette il thread in attesa per un tempo pari ai millisecondi specificati.
 *
 * @param milliseconds quanti millisecondi attendere
 * @return 0 in caso di successo, -1 altrimenti
 */
int msleep(int milliseconds);

#endif //UTILS_H
