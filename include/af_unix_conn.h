#ifndef AF_UNIX_CONN_H
#define AF_UNIX_CONN_H

#define CONN_INTERVAL 1000  //intervallo di tempo tra due tentativi di connessione
#define CONN_ATTEMPTS 3     //massimo numero di tentativi di connessione
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

#endif //AF_UNIX_CONN_H
