#include "../libs/utils/include/utils.h"
#include "../include/af_unix_conn.h"
#include "../include/scfiles.h"
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>

int accept_socket_conn(void) {
    int fd_skt, fd_store, err;
    struct timeval timeout = {MS_TO_SEC(CONN_TIMEOUT), MS_TO_USEC(CONN_TIMEOUT)};
    fd_set set;
    struct sockaddr_un sa;
    strncpy(sa.sun_path, SOCKNAME, UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;

    MINUS1(fd_skt = socket(AF_UNIX, SOCK_STREAM, 0), return -1)
    MINUS1(bind(fd_skt, (struct sockaddr *) &sa, sizeof(sa)), return -1)
    MINUS1(listen(fd_skt, SOMAXCONN), return -1)
    FD_ZERO(&set);
    FD_SET(fd_skt, &set);
    MINUS1(err = select(fd_skt + 1, &set, NULL, NULL, &timeout), return -1)
    if (err == 0) {
        errno = ETIMEDOUT;
        fd_store = -1;
    } else {
        MINUS1(fd_store = accept(fd_skt, NULL, 0), return -1)
    }
    close(fd_skt);
    unlink(SOCKNAME);

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
        if (errno == ENOENT) {
            MINUS1(msleep(1000L),return -1) /* sock ancora non esiste, aspetto CONN_INTERVAL millisecondi e poi riprovo */
        } else {
            return -1;
        }
    }
    return fd_skt;
}

int send_via_socket(int fd, msg_header_t *msg_hdr, int params, ...) {
    int *param;
    va_list list;
    va_start(list, params);
    //Invio l'header
    MINUS1(writen(fd, msg_hdr, sizeof(msg_header_t)), return -1)
    //Invio i parametri
    while(params > 0) {
        param = va_arg(list, int*);
        MINUS1(writen(fd, param, sizeof(int)), return -1)
        params--;
    }
    va_end(list);
    return 0;
}
