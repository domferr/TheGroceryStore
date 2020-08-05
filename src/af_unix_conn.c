#include "../include/utils.h"
#include "../include/af_unix_conn.h"
#include "../include/scfiles.h"
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
            MINUS1(msleep(CONN_INTERVAL),return -1) /* sock ancora non esiste, aspetto CONN_INTERVAL millisecondi e poi riprovo */
        } else {
            return -1;
        }
    }
    return fd_skt;
}

int ask_exit_permission(int fd, int client_id) {
    msg_header_t msg_hdr = head_ask_exit;
    MINUS1(writen(fd, &msg_hdr, sizeof(msg_header_t)), return -1)
    MINUS1(writen(fd, &client_id, sizeof(int)), return -1)
    return 0;
}

int notify(int fd, int cassa_id, int queue_len) {
    msg_header_t msg_hdr = head_notify;
    MINUS1(writen(fd, &msg_hdr, sizeof(msg_header_t)), return -1)
    MINUS1(writen(fd, &cassa_id, sizeof(int)), return -1)
    MINUS1(writen(fd, &queue_len, sizeof(int)), return -1)
    return 0;
}