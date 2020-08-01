#include "../include/utils.h"
#include "../include/af_unix_conn.h"
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int accept_socket_conn(void) {
    int fd_skt, fd_store;
    struct sockaddr_un sa;
    strncpy(sa.sun_path, SOCKNAME, UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;
    MINUS1(fd_skt = socket(AF_UNIX, SOCK_STREAM, 0), return -1)
    MINUS1(bind(fd_skt, (struct sockaddr *) &sa, sizeof(sa)), return -1)
    MINUS1(listen(fd_skt, SOMAXCONN), return -1)
    MINUS1(fd_store = accept(fd_skt, NULL, 0), return -1)
    unlink(SOCKNAME);
    close(fd_skt);
    return fd_store;
}

int connect_via_socket(void) {
    int fd_skt;
    struct sockaddr_un sa;
    strncpy(sa.sun_path, SOCKNAME, UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;
    MINUS1(fd_skt = socket(AF_UNIX, SOCK_STREAM, 0), return -1)
    //Avvia una connessione con il direttore via socket AF_UNIX
    while (connect(fd_skt, (struct sockaddr *) &sa, sizeof(sa)) == -1 ) {
        if (errno == ENOENT)
            msleep(CONN_INTERVAL); /* sock non esiste, aspetto CONN_INTERVAL millisecondi e poi riprovo */
        else
            return -1;
    }
    return fd_skt;
}