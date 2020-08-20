#ifndef STORE_H
#define STORE_H

#include "storetypes.h"
#define ISOPEN(st) (st) == open_state

/**
 * Crea la struttura dati dello store. Richiede il numero massimo di clienti che possono esserci contemporaneamente
 * all'interno del supermercato.
 *
 * @param c numero massimo di clienti che possono esserci contemporaneamente all'interno del supermercato
 * @param e quanto è grande il gruppo di clienti che può entrare nel supermercato
 * @return puntatore alla struttura dati allocata, NULL in caso di errore e setta errno
 */
store_t *store_create(size_t c, size_t e);

/**
 * Distrugge la struttura del supermercato passata per argomento. Il puntatore passato, dopo l'esecuzione di questa
 * funzione non ha più significato. La memoria viene totalmente liberata.
 *
 * @param store struttura dati del supermercato di cui liberare la memoria
 * @return 0 se la distruzione è avvenuta con successo, -1 altrimenti e setta errno
 */
int store_destroy(store_t *store);

/**
 * Aggiorna il puntatore passato per argomento con lo stato del supermercato. Ritorna 0 in caso di successo, -1 altrimenti
 * @param store struttura dati del supermercato
 * @param state puntatore alla variabile da aggiornare con lo stato del supermercato
 * @return 0 in caso di successo, -1 altrimenti
 */
int get_store_state(store_t *store, store_state *st_state);

/**
 * Chiude il supermercato. Ritorna 0 in caso di successo, altrimenti ritorna -1 ed imposta errno.
 *
 * @param store struttura dati del supermercato
 * @param closing_state tipo di chiusura voluta
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno
 */
int close_store(store_t *store, store_state closing_state);

/**
 * Entra nel supermercato. Chiamata bloccante, in quanto i clienti non possono sempre entrare nel supermercato ma gli
 * viene concesso l'accesso se nel supermercato ci sono C-E clienti. I clienti entrano a gruppi di E clienti.
 *
 * @param store struttura dati del supermercato
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno
 */
int enter_store(store_t *store);

/**
 * Lascia il supermercato. Si occupa di fare entrare i clienti se sono usciti E clienti. Ritorna 0 in caso di successo,
 * altrimenti in caso di errore ritorna -1 ed imposta errno
 *
 * @param store struttura dati del supermercato
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno
 */
int leave_store(store_t *store);

/**
 * Invia una richiesta di permesso di uscita. Utilizzato dal thread cliente per richiedere al direttore il permesso per
 * uscire dal supermercato.
 *
 * @param fd descrittore del file utilizzato per la comunicazione con il direttore
 * @param client_id identificatore univoco del thread cliente
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno
 */
int ask_exit_permission(int client_id);

/**
 * Notifica al direttore che nella cassa identificata dal parametro cassa_id ci sono queue_len clienti in coda.
 *
 * @param fd file descriptor per la comunicazione via socket AX_UNIX
 * @param cassa_id identificatore della cassa
 * @param queue_len quanti clienti sono in coda in cassa
 * @return 0 in caso di successo, -1 altrimenti e imposta errno
 */
int notify(int cassa_id, int queue_len);

#endif //STORE_H
