#ifndef AF_UNIX_CONN_H
#define AF_UNIX_CONN_H

#define CONN_TIMEOUT 10000  //Massimo tempo, espresso in millisecondi, per avviare una connessione socket af unix
#define UNIX_PATH_MAX 108
#define SOCKNAME "./sockfile.sock"

/**
 * Headers utilizzati nei messaggi scambiati via socket AF_UNIX. L'header viene utilizzato per identificare la tipologia
 * del messaggio. In base all'header, i parametri assumono uno specifico significato.
 */
typedef enum {
    head_notify = 1,    // i parametri sono {numero cassa, numero di clienti in coda}
    head_open,          // il parametro è il numero della cassa da aprire
    head_close,         // il parametro è il numero della cassa da chiudere
    head_ask_exit,      // il parametro è l'identificatore univoco del cliente che vuole uscire
    head_can_exit       // i parametri sono {id univoco cliente, 1 se il cliente può uscire dal supermercato, 0 altrimenti}
} msg_header_t;

/**
 * Crea un socket AF_UNIX ed accetta una connessione su di esso. Ritorna il file descriptor del client che ha accettato
 * la connessione e chiude il socket.
 *
 * @return file descriptor del client che ha accettato la connessione, -1 in caso di errore e imposta errno
 */
int accept_socket_conn(void);

/**
 * Cerca di connettersi via socket AF_UNIX. Il tentativo di connessione viene svolto ad intervalli di 1 secondo.
 * Ritorna il file descriptor da utilizzare per la comunicazione con il server oppure -1 in caso di errore
 * ed imposta errno.
 *
 * @return il file descriptor per comunicare con il server oppure -1 in caso di errore ed imposta errno
 */
int connect_via_socket(void);

/**
 * Scrive il messaggio da inviare sul file descriptor passato per argomento. Il messaggio è preceduto dall'header passato
 * per argomento. I parametri del messaggio, se presenti, vengono scritti dopo l'header del messaggio. L'header identifica
 * il messaggio e viene utilizzato per la corretta lettura da parte del ricevente.
 *
 * @param fd file descriptor sul quale scrivere il messaggio
 * @param msg_hdr header del messaggio da inviare
 * @param params quanti sono i parametri da inviare
 * @param ... parametri da inviare, se presenti
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno
 */
int send_via_socket(int fd, msg_header_t *msg_hdr, int params, ...);

#endif //AF_UNIX_CONN_H
