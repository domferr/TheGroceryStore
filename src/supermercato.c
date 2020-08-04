#include "../include/sig_handling.h"
#include "../include/config.h"
#include "../include/utils.h"
#include "../include/af_unix_conn.h"
#include "../include/scfiles.h"
#include "../include/store.h"
#include "../include/threadpool.h"
#include "../include/client.h"
#include "../include/cassiere.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/select.h>

//#define DEBUGSUPERM

static thread_pool_t *run_clients(store_t *store, config_t *config, int fd, pthread_mutex_t *fd_mtx);
static thread_pool_t *run_cassieri(store_t *store, config_t *config, int fd, pthread_mutex_t *fd_mtx);

int main(int argc, char **args) {
    int err, fd_skt, sigh_pipe[2], run = 1, param1, param2;
    pthread_mutex_t fd_mtx;
    size_t i;
    pthread_t sig_handler_thread;
    msg_header_t msg_hdr;
    fd_set set, rd_set;
    config_t *config;
    store_t *store;
    store_state closing_state;
    thread_pool_t *clients_pool;
    thread_pool_t *cassieri_pool;

    if (argc != 2) {
        printf("Usage: Il processo supermercato deve essere lanciato dal direttore\n");
        exit(EXIT_FAILURE);
    }
    PTH(err, pthread_mutex_init(&(fd_mtx), NULL), perror("mutex init"); exit(EXIT_FAILURE))
    //Stabilisco connessione via socket AF_UNIX con il direttore
    MINUS1(fd_skt = connect_via_socket(), perror("connect_via_socket"); exit(EXIT_FAILURE))
    //Gestione dei segnali mediante thread apposito
    MINUS1(pipe(sigh_pipe), perror("pipe"); exit(EXIT_FAILURE))
    MINUS1(handle_signals(&sig_handler_thread, &thread_sig_handler_fun, (void*)sigh_pipe), perror("handle_signals"); exit(EXIT_FAILURE))
#ifdef DEBUGSUPERM
    printf("Connesso con il direttore via socket AF_UNIX\n");
#endif
    //Leggo file di configurazione
    EQNULL(config = load_config(args[1]), perror("load_config"); exit(EXIT_FAILURE))
    if (!validate(config)) exit(EXIT_FAILURE);

    FD_ZERO(&set);  //azzero il set
    FD_SET(fd_skt, &set); //imposto il descrittore per comunicare con il supermercato via socket AF_UNIX
    FD_SET(sigh_pipe[0], &set); //imposto il descrittore per comunicare via pipe con il thread signal handler

    EQNULL(store = store_create(config->c, config->e), perror("malloc"); exit(EXIT_FAILURE))
    EQNULL(cassieri_pool = run_cassieri(store, config, fd_skt, &fd_mtx), perror("run cassieri"); exit(EXIT_FAILURE))
    EQNULL(clients_pool = run_clients(store, config, fd_skt, &fd_mtx), perror("run clients"); exit(EXIT_FAILURE))

    while (run) {
        rd_set = set;
        MINUS1(select(sigh_pipe[1]+1, &rd_set, NULL, NULL, NULL), perror("select"); exit(EXIT_FAILURE))
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
        } else {
            MINUS1(err = readn(fd_skt, &msg_hdr, sizeof(msg_header_t)), perror("readn"); exit(EXIT_FAILURE))
            if (err == 0) { //EOF quindi il processo direttore è terminato!
                run = 0;
                err = pthread_kill(sig_handler_thread, SIGINT);
                if (err == -1 && errno != ESRCH) {   //Allora il thread handler non ha già gestito il segnale dal processo direttore
                    //Si tratta di terminazione imprevista quindi mando il segnale al thread signal handler
                    PTH(err, pthread_kill(sig_handler_thread, SIGINT), perror("pthread_kill"); exit(EXIT_FAILURE))
                    break;
                } else if (err == -1) {
                    perror("pthread kill");
                    exit(EXIT_FAILURE);
                }
            }
            switch (msg_hdr) {
                case head_open:
                    MINUS1(readn(fd_skt, &param1, sizeof(int)), perror("readn"); exit(EXIT_FAILURE))
#ifdef DEBUGSUPERM
                    printf("Il direttore dice di aprire la cassa %d\n", param1);
#endif
                    MINUS1(set_cassa_state(cassieri_pool->args[param1], 1), perror("set cassa state"); exit(EXIT_FAILURE))
                    break;
                case head_close:
                    MINUS1(readn(fd_skt, &param1, sizeof(int)), perror("readn"); exit(EXIT_FAILURE))
#ifdef DEBUGSUPERM
                    printf("Il direttore dice di chiudere la cassa %d\n", param1);
#endif
                    MINUS1(set_cassa_state(cassieri_pool->args[param1], 0), perror("set cassa state"); exit(EXIT_FAILURE))
                    break;
                case head_can_exit:
                    MINUS1(readn(fd_skt, &param1, sizeof(int)), perror("readn"); exit(EXIT_FAILURE))
                    MINUS1(readn(fd_skt, &param2, sizeof(int)), perror("readn"); exit(EXIT_FAILURE))
                    MINUS1(set_exit_permission((clients_pool->args)[param1], param2), perror("set exit permission"); exit(EXIT_FAILURE))
#ifdef DEBUGSUPERM
                    printf("Il direttore dice che il cliente %d ", param1);
                    if (!param2)
                        printf("non ");
                    printf("può uscire!\n");
#endif
                    break;
                default:
                    break;
            }
        }
    }

    //chiudo il supermercato e blocco gli ingressi
    MINUS1(close_store(store, closing_state), perror("close_store"); exit(EXIT_FAILURE))
    //sveglio i clienti in attesa di uscita permettendogli di uscire
    for (i = 0; i < clients_pool->size; ++i) {
        MINUS1(set_exit_permission((clients_pool->args)[i], 1), perror("set exit permission"); exit(EXIT_FAILURE))
    }
    //sveglio tutti i cassieri che sono dormienti
    for (i = 0; i < cassieri_pool->size; ++i) {
        MINUS1(cassiere_wake_up(cassieri_pool->args[i]), perror("cassiere quit"); exit(EXIT_FAILURE))
    }
    //join sul thread signal handler
    PTH(err, pthread_join(sig_handler_thread, NULL), perror("join thread handler"); exit(EXIT_FAILURE))
    MINUS1(thread_pool_join(cassieri_pool), perror("join cassieri threads"); exit(EXIT_FAILURE))
    MINUS1(thread_pool_join(clients_pool), perror("join clients threads"); exit(EXIT_FAILURE))
    close(fd_skt);
    close(sigh_pipe[0]);
    close(sigh_pipe[1]);
    //cleanup
    store_destroy(store);
    free_config(config);
    for (i = 0; i < cassieri_pool->size; ++i) {
        MINUS1(cassiere_destroy(cassieri_pool->args[i]), perror("cassiere destroy"); exit(EXIT_FAILURE))
    }
    MINUS1(thread_pool_free(cassieri_pool), perror("thread pool free"); exit(EXIT_FAILURE))
    for (i = 0; i < clients_pool->size; ++i) {
        MINUS1(client_destroy(clients_pool->args[i]), perror("client destroy"); exit(EXIT_FAILURE))
    }
    MINUS1(thread_pool_free(clients_pool), perror("thread pool free"); exit(EXIT_FAILURE))
    PTH(err, pthread_mutex_destroy(&(fd_mtx)), return -1)
#ifdef DEBUGSUPERM
    printf("Supermercato: Termino!\n");
#endif
    return 0;
}

static thread_pool_t *run_clients(store_t *store, config_t *config, int fd, pthread_mutex_t *fd_mtx) {
    int i;
    client_t *arg;
    thread_pool_t *pool = thread_pool_create(config->c);
    EQNULL(pool, return NULL)
    for (i = 0; i < config->c; ++i) {
        EQNULL(arg = alloc_client(i, store, config->t, config->p, config->s, config->k, fd, fd_mtx), return NULL)
        MINUS1(thread_create(pool, client_thread_fun, arg), return NULL)
    }
    return pool;
}

static thread_pool_t *run_cassieri(store_t *store, config_t *config, int fd, pthread_mutex_t *fd_mtx) {
    int i, is_open;
    cassiere_t *arg;
    thread_pool_t *pool = thread_pool_create(config->k);
    EQNULL(pool, return NULL)
    for (i = 0; i < config->k; ++i) {
        is_open = i < config->ka;   //all'inizio le prime ka casse sono aperte
        EQNULL(arg = alloc_cassiere(i, store, is_open, fd, fd_mtx, config->kt, config->d), return NULL)
        MINUS1(thread_create(pool, cassiere_thread_fun, arg), return NULL)
    }
    return pool;
}