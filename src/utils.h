#ifndef UTILS_H
#define UTILS_H

#define _POSIX_C_SOURCE 200809L
#include <time.h>
#include <errno.h>


#define EQNULL(w, then)     \
        if (w == NULL) {    \
            then;           \
        }

#define MINUS1(w, then) \
        if (w == -1) {  \
            then;       \
        }

#define NOTZERO(w, then)    \
        if (w != 0) {       \
            then;           \
        }

#define PTH(e, pcall, then)         \
        if ((e = pcall) != 0) {     \
            errno = e;              \
            then;                   \
        }

/**
 * Conversione da millisecondi a secondi
 */
#define MS_TO_SEC(ms) ms/1000

/**
 * Quanti nanosecondi ci sono nei millisecondi specificati
 */
#define MS_TO_NANOSEC(ms) (milliseconds%1000)*1000000

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

/**
 * Ritorna il tempo passato in millisecondi dai due istanti definiti da start ed end
 *
 * @param start istante di inizio
 * @param end istante di fine
 * @return ritorna la durata in millisecondi tra l'istante di inizio e l'istante di fine
 */
long get_elapsed_milliseconds(struct timespec *start, struct timespec *end);

#endif //UTILS_H
