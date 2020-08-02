#include "../include/sig_handling.h"
#include "../include/config.h"
#include "../include/utils.h"
#include "../include/af_unix_conn.h"
#include "../include/scfiles.h"
#include "../include/store.h"
#include "../include/threadpool.h"
#include "../include/client.h"
#include "storetypes.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/select.h>

#define DBGPRINTF(str) printf("\rSUPERMERCATO: "str);

/**
 * Notifica al direttore che nella cassa identificata dal parametro cassa_id ci sono queue_len clienti in coda.
 *
 * @param fd file descriptor per la comunicazione via socket AX_UNIX
 * @param cassa_id identificatore della cassa
 * @param queue_len quanti clienti sono in coda in cassa
 * @return 0 in caso di successo, -1 altrimenti e imposta errno
 */
static int notify(int fd, int cassa_id, int queue_len);

static int ask_exit_permission(int fd_skt, int client_id);

static thread_pool_t *run_clients(store_t *store, config_t *config, int *shared_pipe);


int main(int argc, char **args) {
    int err, fd_skt, sigh_pipe[2], shared_pipe[2], run = 1, param1, param2;
    pthread_t sig_handler_thread;
    msg_header_t msg_hdr;
    fd_set set, rd_set;
    config_t *config;
    store_t *store;
    store_state closing_state;
    void **clients_retval;
    thread_pool_t *clients_pool;

    if (argc != 2) {
        printf("Usage: Il processo supermercato deve essere lanciato dal direttore\n");
        exit(EXIT_FAILURE);
    }
    //Stabilisco connessione via socket AF_UNIX con il direttore
    MINUS1(fd_skt = connect_via_socket(), perror("connect_via_socket"); exit(EXIT_FAILURE))
    //Gestione dei segnali mediante thread apposito
    MINUS1(pipe(sigh_pipe), perror("pipe"); exit(EXIT_FAILURE))
    MINUS1(pipe(shared_pipe), perror("pipe"); exit(EXIT_FAILURE))
    MINUS1(handle_signals(&sig_handler_thread, &thread_sig_handler_fun, (void*)sigh_pipe), perror("handle_signals"); exit(EXIT_FAILURE))
    DBGPRINTF("Connesso con il direttore via socket AF_UNIX\n");
    //Leggo file di configurazione
    EQNULL(config = load_config(args[1]), perror("load_config"); exit(EXIT_FAILURE))
    if (!validate(config)) exit(EXIT_FAILURE);

    FD_ZERO(&set);  //azzero il set
    FD_SET(fd_skt, &set); //imposto il descrittore per comunicare con il supermercato via socket AF_UNIX
    FD_SET(sigh_pipe[0], &set); //imposto il descrittore per comunicare via pipe con il thread signal handler
    FD_SET(shared_pipe[0], &set); //imposto il descrittore per comunicare via pipe con notificatori e clienti

    EQNULL(store = store_create(config->c, config->e), perror("malloc"); exit(EXIT_FAILURE))
    EQNULL(clients_retval = (void*) malloc(sizeof(void*)*config->c), perror("malloc"); exit(EXIT_FAILURE))
    EQNULL(clients_pool = run_clients(store, config, shared_pipe), perror("run clients"); exit(EXIT_FAILURE))

    while (run) {
        rd_set = set;
        MINUS1(select(shared_pipe[1]+1, &rd_set, NULL, NULL, NULL), perror("select"); exit(EXIT_FAILURE))
        if (FD_ISSET(sigh_pipe[0], &rd_set)) {
            run = 0;
            MINUS1(readn(sigh_pipe[0], &param1, sizeof(int)), perror("readn"); exit(EXIT_FAILURE))
            switch (param1) {
                case SIGINT:
                case SIGQUIT:
                    closing_state = closed_fast_state;
                    break;
                case SIGHUP:
                    closing_state = closed_state;
                    break;
                default:
                    break;
            }
        } if (FD_ISSET(shared_pipe[0], &rd_set)) {
            MINUS1(err = readn(shared_pipe[0], &msg_hdr, sizeof(msg_header_t)), perror("readn"); exit(EXIT_FAILURE))
            switch (msg_hdr) {
                case head_notify:
                    MINUS1(err = readn(shared_pipe[0], &param1, sizeof(int)), perror("readn"); exit(EXIT_FAILURE))
                    MINUS1(err = readn(shared_pipe[0], &param2, sizeof(int)), perror("readn"); exit(EXIT_FAILURE))
                    MINUS1(notify(fd_skt, param1, param2), perror("notify"); exit(EXIT_FAILURE))
                    break;
                case head_ask_exit:
                    MINUS1(err = readn(shared_pipe[0], &param1, sizeof(int)), perror("readn"); exit(EXIT_FAILURE))
                    MINUS1(ask_exit_permission(fd_skt, param1), perror("ask exit permission"); exit(EXIT_FAILURE))
                    break;
                default:
                    break;
            }
        } else {
            printf("Comunicazione dal direttore\n");
            MINUS1(err = readn(fd_skt, &msg_hdr, sizeof(msg_header_t)), perror("readn"); exit(EXIT_FAILURE))
            if (err == 0) { //EOF quindi il processo direttore è terminato!
                run = 0;
                err = pthread_kill(sig_handler_thread, SIGINT);
                if (err == -1 && errno != ESRCH)   //Allora il thread handler non ha già gestito il segnale dal processo direttore
                    PTH(err, pthread_kill(sig_handler_thread, SIGINT), perror("pthread_kill"); exit(EXIT_FAILURE))
                break;
            }
            switch (msg_hdr) {
                case head_open:
                    MINUS1(readn(fd_skt, &param1, sizeof(int)), return -1)
                    printf("Il direttore dice di aprire la cassa %d\n", param1);
                    break;
                case head_close:
                    MINUS1(readn(fd_skt, &param1, sizeof(int)), return -1)
                    printf("Il direttore dice di chiudere la cassa %d\n", param1);
                    break;
                case head_can_exit:
                    MINUS1(readn(fd_skt, &param1, sizeof(int)), return -1)
                    printf("Il direttore dice che il cliente %d ", param1);
                    MINUS1(readn(fd_skt, &param2, sizeof(int)), return -1)
                    if (!param2)
                        printf("non ");
                    printf("può uscire!\n");
                    break;
                default:
                    break;
            }
        }
    }

    //chiudo il supermercato e blocco gli ingressi
    MINUS1(close_store(store, closing_state), perror("close_store"); exit(EXIT_FAILURE))
    //join sul thread signal handler
    PTH(err, pthread_join(sig_handler_thread, NULL), perror("join thread handler"); exit(EXIT_FAILURE))
    MINUS1(thread_pool_join(clients_pool, clients_retval), perror("join clients threads"); exit(EXIT_FAILURE))
    close(fd_skt);
    close(sigh_pipe[0]);
    close(sigh_pipe[1]);
    close(shared_pipe[0]);
    close(shared_pipe[1]);
    //cleanup
    store_destroy(store);
    free_config(config);
    thread_pool_free(clients_pool);
    free(clients_retval);
    DBGPRINTF("Termino!\n");
    return 0;
}

static thread_pool_t *run_clients(store_t *store, config_t *config, int *shared_pipe) {
    int i;
    client_t *arg;
    thread_pool_t *pool = thread_pool_create(config->c);
    EQNULL(pool, return NULL)
    for (i = 0; i < config->c; ++i) {
        arg = alloc_client(i, store, config->t, config->p, config->s, shared_pipe, config->k);
        MINUS1(thread_create(pool, client_thread_fun, arg), return NULL)
    }
    return pool;
}

static int notify(int fd, int cassa_id, int queue_len) {
    msg_header_t msg_hdr = head_notify;
    MINUS1(writen(fd, &msg_hdr, sizeof(msg_header_t)), return -1)
    MINUS1(writen(fd, &cassa_id, sizeof(int)), return -1)
    MINUS1(writen(fd, &queue_len, sizeof(int)), return -1)
    return 0;
}

static int ask_exit_permission(int fd_skt, int client_id) {
    msg_header_t msg_hdr = head_ask_exit;
    MINUS1(writen(fd_skt, &msg_hdr, sizeof(msg_header_t)), return -1)
    MINUS1(writen(fd_skt, &client_id, sizeof(int)), return -1)
    return 0;
}