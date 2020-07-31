#ifndef AF_UNIX_CONN_H
#define AF_UNIX_CONN_H

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
