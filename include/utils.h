/**
 * Contiene macro utili per il controllo degli errori e funzioni utili per l'intero programma
 */

#ifndef UTILS_H
#define UTILS_H

#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <time.h>
#include <pthread.h>

#define EQNULL(w, then)     \
        if ((w) == NULL) {  \
            then;           \
        }

#define MINUS1(w, then)     \
        if ((w) == -1) {    \
            then;           \
        }

#define NOTZERO(w, then)    \
        if ((w) != 0) {     \
            then;           \
        }

#define PTH(e, pcall, then)         \
        if (((e) = (pcall)) != 0) { \
            errno = e;              \
            then;                   \
        }

/**
 * Conversione da millisecondi a secondi
 */
#define MS_TO_SEC(ms) ((ms)/1000L)

/**
 * Quanti nanosecondi ci sono nei millisecondi specificati
 */
#define MS_TO_NANOSEC(ms) (((ms)%1000L)*1000000L)

/**
 * Genera numero random tra min (incluso) e max (non incluso)
 */
#define RANDOM(seed_ptr, min, max) ((rand_r(&(seed_ptr))%((max)-(min))) + (min))

//https://gcc.gnu.org/onlinedocs/cpp/Variadic-Macros.html
#ifdef DEBUGGING
#if DEBUGGING
#define DEBUG(...) \
    do { printf(__VA_ARGS__); } while(0);
#else
#define DEBUG(str, ...)
#endif
#else
#define DEBUG(str, ...)
#endif

/**
 * Mette il thread in attesa per un tempo pari ai millisecondi specificati.
 *
 * @param milliseconds quanti millisecondi attendere
 * @return 0 in caso di successo, -1 altrimenti
 */
int msleep(int milliseconds);

/**
 * Ritorna il tempo passato in millisecondi dall'istante indicato dal parametro start.
 *
 * @param start istante di inizio
 * @return ritorna la durata in millisecondi dall'istante passato per argomento, -1 in caso di errore ed imposta errno
 */
long elapsed_time(struct timespec *start);

/**
 * Ritorna 0 oppure 1 in base alla probabilità passata per argomento. Ad esempio, se si passa 50 come valore dell'intero
 * percent, allora la funzione ritorna 1 con il 50% di probabilità.
 *
 * @param seed utilizzato per la generazione di valori pseudo casuali
 * @param percent probabilità voluta
 * @return 1 oppure 0 in base alla generazione del numero random ed in base al valore di percent passato
 */
int probability(unsigned int seed, int percent);

#endif //UTILS_H
