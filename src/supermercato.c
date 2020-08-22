#define DEBUGGING 0
#include "../include/sig_handling.h"
#include "../include/config.h"
#include "../include/utils.h"
#include "../include/af_unix_conn.h"
#include "../include/scfiles.h"
#include "../include/store.h"
#include "../include/threadpool.h"
#include "../include/client.h"
#include "../include/cassiere.h"
#include "../include/log.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/select.h>

static thread_pool_t *run_clients(store_t *store, config_t *config, cassiere_t **casse);
static thread_pool_t *run_cassieri(store_t *store, config_t *config);

static pthread_mutex_t mtx_skt = PTHREAD_MUTEX_INITIALIZER; //Mutex per comunicare con il direttore in mutua esclusione
static int fd_skt;  //descrittore per comunicazione via socket af_unix con il direttore

int main(int argc, char **args) {
    int err, sigh_pipe[2], run = 1, param1, param2;
    size_t i;
    msg_header_t msg_hdr;
    fd_set set, rd_set;
    config_t *config;
    store_t *store;
    store_state closing_state;
    thread_pool_t *clients_pool;
    thread_pool_t *cassieri_pool;
    list_t *clients_stats;

    if (argc != 2) {
        printf("Usage: Il processo supermercato deve essere lanciato dal direttore\n");
        exit(EXIT_FAILURE);
    }

    //Gestione dei segnali mediante thread apposito
    MINUS1ERR(pipe(sigh_pipe), exit(EXIT_FAILURE))
    MINUS1ERR(handle_signals(&thread_sig_handler_fun, (void*)sigh_pipe), exit(EXIT_FAILURE))
    //Stabilisco connessione via socket AF_UNIX con il direttore
    MINUS1ERR(fd_skt = connect_via_socket(), exit(EXIT_FAILURE))
    DEBUG("%s\n", "Connesso con il direttore via socket AF_UNIX");
    //Leggo file di configurazione
    EQNULLERR(config = load_config(args[1]), exit(EXIT_FAILURE))
    if (!validate(config)) exit(EXIT_FAILURE);

    FD_ZERO(&set);  //azzero il set
    FD_SET(fd_skt, &set); //imposto il descrittore per comunicare con il supermercato via socket AF_UNIX
    FD_SET(sigh_pipe[0], &set); //imposto il descrittore per comunicare via pipe con il thread signal handler

    EQNULLERR(store = store_create(config->c, config->e), exit(EXIT_FAILURE))
    EQNULLERR(cassieri_pool = run_cassieri(store, config), exit(EXIT_FAILURE))
    EQNULLERR(clients_pool = run_clients(store, config, (cassiere_t **) cassieri_pool->args), exit(EXIT_FAILURE))

    while (run) {
        rd_set = set;
        MINUS1ERR(select(fd_skt+1, &rd_set, NULL, NULL, NULL), exit(EXIT_FAILURE))
        if (FD_ISSET(sigh_pipe[0], &rd_set)) {
            run = 0;
            MINUS1ERR(readn(sigh_pipe[0], &param1, sizeof(int)), exit(EXIT_FAILURE))
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
            MINUS1ERR(err = readn(fd_skt, &msg_hdr, sizeof(msg_header_t)), exit(EXIT_FAILURE))
            if (err == 0) { //EOF quindi il processo direttore è terminato!
                run = 0;
                closing_state = closed_fast_state;  //terminazione imprevista quindi chiusura rapida
            }
            switch (msg_hdr) {
                case head_open:
                    MINUS1ERR(readn(fd_skt, &param1, sizeof(int)), exit(EXIT_FAILURE))
                    DEBUG("Il direttore dice di aprire la cassa %d\n", param1);
                    MINUS1ERR(set_cassa_state(cassieri_pool->args[param1], 1), exit(EXIT_FAILURE))
                    break;
                case head_close:
                    MINUS1ERR(readn(fd_skt, &param1, sizeof(int)), exit(EXIT_FAILURE))
                    DEBUG("Il direttore dice di chiudere la cassa %d\n", param1);
                    MINUS1ERR(set_cassa_state(cassieri_pool->args[param1], 0), exit(EXIT_FAILURE))
                    break;
                case head_can_exit:
                    MINUS1ERR(readn(fd_skt, &param1, sizeof(int)), exit(EXIT_FAILURE))
                    MINUS1ERR(readn(fd_skt, &param2, sizeof(int)), exit(EXIT_FAILURE))
                    MINUS1ERR(set_exit_permission((clients_pool->args)[param1], param2), exit(EXIT_FAILURE))
                    if (param2) {
                        DEBUG("Il direttore dice che il cliente %d può uscire\n", param1)
                    } else {
                        DEBUG("Il direttore dice che il cliente %d non può uscire\n", param1)
                    }
                    break;
                default:
                    break;
            }
        }
    }

    //chiudo il supermercato e blocco gli ingressi
    MINUS1ERR(close_store(store, closing_state), exit(EXIT_FAILURE))
    DEBUG("%s\n","Supermercato chiuso")
    //sveglio i clienti in attesa di uscita permettendogli di uscire
    for (i = 0; i < clients_pool->size; ++i) {
        MINUS1ERR(set_exit_permission((clients_pool->args)[i], 1), exit(EXIT_FAILURE))
    }
    //sveglio tutti i cassieri che sono dormienti
    for (i = 0; i < cassieri_pool->size; ++i) {
        MINUS1ERR(cassiere_wake_up(cassieri_pool->args[i]), exit(EXIT_FAILURE))
    }
    //join thread cassieri e thread clienti
    MINUS1ERR(thread_pool_join(cassieri_pool), exit(EXIT_FAILURE))
    DEBUG("%s\n","Tutti i cassieri sono terminati")
    MINUS1ERR(thread_pool_join(clients_pool), exit(EXIT_FAILURE))
    DEBUG("%s\n","Tutti i clienti sono terminati")
    //chiudo il socket e la pipe
    PTHERR(err, pthread_mutex_lock(&mtx_skt), exit(EXIT_FAILURE))
    close(fd_skt);
    PTHERR(err, pthread_mutex_unlock(&mtx_skt), exit(EXIT_FAILURE))
    close(sigh_pipe[0]);
    close(sigh_pipe[1]);
    //cleanup dei clienti e prendo le loro statistiche
    EQNULLERR(clients_stats = queue_create(), exit(EXIT_FAILURE))
    for (i = 0; i < clients_pool->size; ++i) {
        MINUS1ERR(client_destroy(clients_pool->args[i]), exit(EXIT_FAILURE))
        merge(clients_stats, clients_pool->retvalues[i]);
    }
    MINUS1ERR(thread_pool_free(clients_pool), exit(EXIT_FAILURE))
    //Scrivo il file di log
    MINUS1ERR(write_log(config->logfilename, clients_stats, (cassa_log_t**) cassieri_pool->retvalues, config->k), exit(EXIT_FAILURE))
    DEBUG("%s\n", "Ho scritto il file di log!")
    //cleanup
    for (i = 0; i < cassieri_pool->size; ++i) {
        MINUS1ERR(cassiere_destroy(cassieri_pool->args[i]), exit(EXIT_FAILURE))
        MINUS1ERR(destroy_cassa_log(cassieri_pool->retvalues[i]), exit(EXIT_FAILURE))
    }
    MINUS1ERR(thread_pool_free(cassieri_pool), exit(EXIT_FAILURE))
    MINUS1ERR(queue_destroy(clients_stats, &free), exit(EXIT_FAILURE))
    MINUS1ERR(store_destroy(store), exit(EXIT_FAILURE))
    free_config(config);
    PTHERR(err, pthread_mutex_destroy(&mtx_skt), exit(EXIT_FAILURE))
    DEBUG("%s\n", "Supermercato: Termino!")
    return 0;
}

static thread_pool_t *run_clients(store_t *store, config_t *config, cassiere_t **casse) {
    int i;
    client_t *arg;
    thread_pool_t *pool = thread_pool_create(config->c);
    EQNULL(pool, return NULL)
    for (i = 0; i < config->c; ++i) {
        EQNULL(arg = alloc_client(i, store, casse, config->t, config->p, config->s, config->k), return NULL)
        MINUS1(thread_create(pool, client_thread_fun, arg), return NULL)
    }
    return pool;
}

static thread_pool_t *run_cassieri(store_t *store, config_t *config) {
    int i, is_open;
    cassiere_t *arg;
    thread_pool_t *pool = thread_pool_create(config->k);
    EQNULL(pool, return NULL)
    for (i = 0; i < config->k; ++i) {
        is_open = i < config->ka;   //all'inizio le prime ka casse sono aperte
        EQNULL(arg = alloc_cassiere(i, store, is_open, config->kt, config->d), return NULL)
        MINUS1(thread_create(pool, cassiere_thread_fun, arg), return NULL)
    }
    return pool;
}

int ask_exit_permission(int client_id) {
    int err;
    msg_header_t msg_hdr = head_ask_exit;
    PTH(err, pthread_mutex_lock(&mtx_skt), return -1)
    MINUS1(send_via_socket(fd_skt, &msg_hdr, 1, &client_id), return -1)
    PTH(err, pthread_mutex_unlock(&mtx_skt), return -1)
    return 0;
}

int notify(int cassa_id, int queue_len) {
    int err;
    msg_header_t msg_hdr = head_notify;
    PTH(err, pthread_mutex_lock(&mtx_skt), return -1)
    MINUS1(send_via_socket(fd_skt, &msg_hdr, 2, &cassa_id, &queue_len), return -1)
    PTH(err, pthread_mutex_unlock(&mtx_skt), return -1)
    return 0;
}