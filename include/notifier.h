#ifndef NOTIFIER_H
#define NOTIFIER_H

#include "storetypes.h"

typedef enum {
    notifier_run,   //il notificatore invia le notifiche
    notifier_pause, //il notificatore non invia le notifiche
    notifier_quit   //stato di terminazione del thread
} notifier_state;

typedef struct {
    cassiere_t *cassiere;   //cassiere assegnato al notificatore
    pthread_mutex_t mutex;
    pthread_cond_t paused;  //il notificatore attende su questa condition variable quando viene messo in pausa
    notifier_state state;   //stato del notificatore
} notifier_t;

/**
 * Crea la struttura dati di un notificatore la quale viene ritornata in caso di successo. In caso di errore, la funzione
 * ritorna NULL.
 *
 * @param cassiere cassiere assegnato al notificatore
 * @param start_notify vale 1 se deve iniziare a notificare, 0 altrimenti
 * @return puntatore alla struttura dati creata in caso di successo, NULL altrimenti ed imposta errno
 */
notifier_t *alloc_notifier(cassiere_t *cassiere, int start_notify);

/**
 * Distrugge la struttura dati di un notificatore, liberandone la memoria. Dopo la chiamata di questa funzione, il
 * puntatore alla struttura dati perde di significato e non deve essere più utilizzato.
 *
 * @param notifier puntatore alla struttura dati del notificatore
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno
 */
int destroy_notifier(notifier_t *notifier);

/**
 * Funzione svolta dal thread notificatore, il quale si occuperà di notificare il direttore riguardo al numero di clienti
 * in coda alla cassa assegnata al notificatore. Il thread viene lanciato, gestito e fatto terminare dal cassiere stesso.
 *
 * @param args argomenti passati alla funzione
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno
 */
void *notifier_thread_fun(void *args);

/**
 * Cambia lo stato di un notificatore con lo stato passato per argomento.
 *
 * @param no puntatore alla struttura dati del notificatore
 * @param new_state nuovo stato del notificatore
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno
 */
int set_notifier_state(notifier_t *no, notifier_state new_state);

#endif //NOTIFIER_H
