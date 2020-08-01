#ifndef SIG_HANDLER_H
#define SIG_HANDLER_H

#define _POSIX_C_SOURCE 200809L
#include <pthread.h>

/**
 * Maschera tutti i segnali e lancia un thread apposito per la gestione dei segnali SIGINT, SIGQUIT e SIGHUP.
 *
 * @param handler viene impostato con il tid del thread creato
 * @param thread_fun funzione svolta dal thread gestore dei segnali
 * @param args argomenti passati al thread gestore dei segnali
 * @return 0 in caso di successo, -1 in caso di errore e imposta errno
 */
int handle_signals(pthread_t *handler, void* (*thread_fun)(void*), void* args);

/**
 * Funzione svolta dal thread gestore dei segnali. Il thread rimane in ascolto dei segnali SIGINT, SIGQUIT, SIGHUP.
 * Una volta ricevuti, i segnali vengono fatti rimbalzare via pipe.
 *
 * @param args argomenti passati alla funzione
 * @return 0 in caso di terminazione con successo
 */
void *thread_sig_handler_fun(void *args);

#endif //SIG_HANDLER_H
