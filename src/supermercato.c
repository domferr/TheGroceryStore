#include "../include/sig_handling.h"
#include "../include/config.h"
#include "../include/utils.h"
#include "../include/af_unix_conn.h"
#include "../include/scfiles.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/select.h>

int main(int argc, char **args) {
    int err, fd_skt, sigh_pipe[2], run = 1, msg_param, sig_arrived, client_id, can_exit;
    pthread_t sig_handler_thread;
    msg_header_t msg_hdr;
    fd_set set, rd_set;
    if (argc != 2) {
        printf("Usage: Il processo supermercato deve essere eseguito dal direttore\n");
        exit(EXIT_FAILURE);
    }
    //Stabilisco connessione via socket AF_UNIX con il direttore
    MINUS1(fd_skt = connect_via_socket(), perror("connect_via_socket"); exit(EXIT_FAILURE))
    //Gestione dei segnali mediante thread apposito
    MINUS1(pipe(sigh_pipe), perror("pipe"); exit(EXIT_FAILURE))
    MINUS1(handle_signals(&sig_handler_thread, &thread_sig_handler_fun, (void*)sigh_pipe), perror("handle_signals"); exit(EXIT_FAILURE))
    printf("SUPERMERCATO: Connesso con il direttore via socket AF_UNIX\n");
    //Leggo file di configurazione
    config_t *config = load_config(args[1]);
    EQNULL(config, exit(EXIT_FAILURE))
    if (!validate(config))
        exit(EXIT_FAILURE);

    msg_hdr = head_notify;
    MINUS1(writen(fd_skt, &msg_hdr, sizeof(msg_header_t)), return -1)
    msg_param = 10;
    MINUS1(writen(fd_skt, &msg_param, sizeof(int)), return -1)
    msg_param = 48;
    MINUS1(writen(fd_skt, &msg_param, sizeof(int)), return -1)

    FD_ZERO(&set);  //azzero il set
    FD_SET(fd_skt, &set); //imposto il descrittore per comunicare con il supermercato via socket AF_UNIX
    FD_SET(sigh_pipe[0], &set); //imposto il descrittore per comunicare via pipe con il thread signal handler

    while (run) {
        rd_set = set;
        MINUS1(select(sigh_pipe[1]+1, &rd_set, NULL, NULL, NULL), perror("select"); exit(EXIT_FAILURE))
        if (FD_ISSET(fd_skt, &rd_set)) {
            printf("Comunicazione dal direttore\n");
            MINUS1(err = readn(fd_skt, &msg_hdr, sizeof(msg_header_t)), perror("readn"); exit(EXIT_FAILURE))
            if (err == 0) { //EOF quindi il processo direttore è terminato!
                run = 0;
                printf("IL DIRETTORE E' STATO TERMINATO!!!");
                PTH(err, pthread_kill(sig_handler_thread, SIGINT), perror("pthread_kill"); exit(EXIT_FAILURE))
            }
            switch (msg_hdr) {
                case head_open:
                    MINUS1(readn(fd_skt, &msg_param, sizeof(int)), return -1)
                    printf("Il direttore dice di aprire la cassa %d\n", msg_param);
                    break;
                case head_close:
                    MINUS1(readn(fd_skt, &msg_param, sizeof(int)), return -1)
                    printf("Il direttore dice di chiudere la cassa %d\n", msg_param);
                    break;
                case head_can_exit:
                    MINUS1(readn(fd_skt, &client_id, sizeof(int)), return -1)
                    printf("Il direttore dice che il cliente %d ", client_id);
                    MINUS1(readn(fd_skt, &can_exit, sizeof(int)), return -1)
                    if (!can_exit)
                        printf("non ");
                    printf("può uscire!\n");
                    break;
                default:
                    break;
            }
        } else {
            run = 0;
            MINUS1(readn(sigh_pipe[0], &sig_arrived, sizeof(int)), perror("readn"); exit(EXIT_FAILURE))
        }
    }
    //join sul thread signal handler
    PTH(err, pthread_join(sig_handler_thread, NULL), perror("join thread handler"); exit(EXIT_FAILURE))
    close(fd_skt);
    close(sigh_pipe[0]);
    close(sigh_pipe[1]);
    //cleanup
    free_config(config);
    printf("\rSUPERMERCATO: Goodbye!\n");
    return 0;
}
