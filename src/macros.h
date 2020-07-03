//
// Created by Domenico on 03/07/2020.
//

#ifndef MACROS_H
#define MACROS_H
/**
 * Controlla se what è uguale a -1. Se si, stampa il messaggio d'errore message e termina
 */
#define NOTMINUS1(what, message, then)    \
        if (what == -1) {                 \
            perror(message);              \
            then;                         \
        }

/**
* Controlla se what è uguale a NULL. Se si, stampa il messaggio d'errore message e continua con then
*/
#define NOTNULL(what, message, then)    \
        if (what == NULL) {             \
            perror(message);            \
            then;                       \
        }


#endif //MACROS_H