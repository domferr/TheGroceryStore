#include "../include/sig_handling.h"
#include "../include/utils.h"
#include "../include/config.h"
#include "../include/af_unix_conn.h"
#include "../include/scfiles.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/select.h>

#define STORE_EXECUTABLE_PATH "./bin/supermercato"
#define STORE_EXECUTABLE_NAME "supermercato"
#define DEFAULT_CONFIG_FILE "./config.txt"
#define ARG_CONFIG_FILE "-c"

#define ERROR_READ_CONFIG_FILE "Impossibile leggere il file di configurazione"
#define MESSAGE_VALID_CONFIG_FILE "File di configurazione: "

/**
 * Esegue il parsing degli argomenti passati al programma via linea di comando. Ritorna il path del file
 * di configurazione indicato dall'utente oppure il path di default se l'utente non ha specificato nulla.
 *
 * @param argc numero di argomenti
 * @param args gli argomenti passati al programma
 * @return il path del file di configurazione indicato dall'utente oppure il path di default se l'utente non lo ha
 * specificato
 */
static char *parse_args(int argc, char **args);

/**
 * Lancia il processo supermercato passando anche il nome del file di configurazione come parametro.
 *
 * @param config_file path del file di configurazione
 * @param pid viene impostato con il process id del processo supermercato.
 * @return 0 in caso di successo, -1 in caso di errore e imposta errno
 */
static int fork_store(char *config_file, pid_t *pid);

int main(int argc, char **args) {
    int err, fd_store, sigh_pipe[2], sig_arrived, msg_param, run = 1;
    fd_set set, rd_set;
    pid_t pid_store;            //pid del processo supermercato
    pthread_t sig_handler_thread;   //thread che si occupa di gestire i segnali
    config_t *config;           //Struttura con i parametri di configurazione
    msg_header_t msg_hdr;

    //Eseguo il parsing del nome del file di configurazione
    char *config_file_path = parse_args(argc, args);
    //Leggo il file di configurazione
    EQNULL(config = load_config(config_file_path), perror(ERROR_READ_CONFIG_FILE); exit(EXIT_FAILURE))
    if (!validate(config)) {
        exit(EXIT_FAILURE);
    }
    printf(MESSAGE_VALID_CONFIG_FILE);
    print_config(config);

    //Lancio il processo supermercato
    MINUS1(fork_store(config_file_path, &pid_store), perror("fork_store"); exit(EXIT_FAILURE))
    printf("DIRETTORE: Connesso con il supermercato via socket AF_UNIX\n");
    MINUS1(pipe(sigh_pipe), perror("pipe"); exit(EXIT_FAILURE))
    //Gestione dei segnali mediante thread apposito
    MINUS1(handle_signals(&sig_handler_thread, &thread_sig_handler_fun, (void*)sigh_pipe), perror("handle_signals"); exit(EXIT_FAILURE))
    //Creo una connessione con il supermercato via socket AF_UNIX
    MINUS1(fd_store = accept_socket_conn(), perror("accept_socket_conn"); exit(EXIT_FAILURE))

    FD_ZERO(&set);  //azzero il set
    FD_SET(fd_store, &set); //imposto il descrittore per comunicare con il supermercato via socket AF_UNIX
    FD_SET(sigh_pipe[0], &set); //imposto il descrittore per comunicare via pipe con il thread signal handler

    while (run) {
        rd_set = set;
        MINUS1(select(fd_store+1, &rd_set, NULL, NULL, NULL), perror("select"); exit(EXIT_FAILURE))
        if (FD_ISSET(fd_store, &rd_set)) {
            printf("Comunicazione dal supermercato\n");
            MINUS1(err = readn(fd_store, &msg_hdr, sizeof(msg_header_t)), perror("readn"); exit(EXIT_FAILURE))
            if (err == 0) { //EOF quindi il processo supermercato è terminato!
                run = 0;
                printf("IL SUPERMERCATO E' STATO TERMINATO!!!");
                PTH(err, pthread_kill(sig_handler_thread, SIGINT), perror("pthread_kill"); exit(EXIT_FAILURE))
            }
            switch (msg_hdr) {
                case head_notify:   //on notification recvd
                    MINUS1(readn(fd_store, &msg_param, sizeof(int)), perror("readn"); exit(EXIT_FAILURE))
                    printf("Ho ricevuto che nella cassa %d ", msg_param);
                    MINUS1(readn(fd_store, &msg_param, sizeof(int)), perror("readn"); exit(EXIT_FAILURE))
                    printf("ci sono %d clienti in coda\n", msg_param);
                    //Rispondo aprendo la cassa
                    msg_hdr = head_open;
                    MINUS1(writen(fd_store, &msg_hdr, sizeof(msg_header_t)), perror("writen"); exit(EXIT_FAILURE))
                    msg_param = 25;
                    MINUS1(writen(fd_store, &msg_param, sizeof(int)), perror("writen"); exit(EXIT_FAILURE))
                    break;
                case head_ask_exit: //on exit request recvd
                    //leggo l'id del cliente che vuole uscire
                    MINUS1(readn(fd_store, &msg_param, sizeof(int)), perror("readn"); exit(EXIT_FAILURE))
                    msg_hdr = head_can_exit;
                    MINUS1(writen(fd_store, &msg_hdr, sizeof(msg_header_t)), perror("writen"); exit(EXIT_FAILURE))
                    //Scrivo l'id del cliente che vuole uscire
                    MINUS1(writen(fd_store, &msg_param, sizeof(int)), perror("writen"); exit(EXIT_FAILURE))
                    msg_param = 0;
                    MINUS1(writen(fd_store, &msg_param, sizeof(int)), perror("writen"); exit(EXIT_FAILURE))
                    break;
                default:
                    break;
            }
        } else {
            run = 0;
            MINUS1(readn(sigh_pipe[0], &sig_arrived, sizeof(int)), perror("readn"); exit(EXIT_FAILURE))
            MINUS1(kill(pid_store, sig_arrived), perror("kill"); exit(EXIT_FAILURE))
        }
    }

    //join sul thread signal handler
    PTH(err, pthread_join(sig_handler_thread, NULL), perror("join thread handler"); exit(EXIT_FAILURE))
    //chiudo i file descriptors
    close(fd_store);
    close(sigh_pipe[0]);
    close(sigh_pipe[1]);
    //memory cleanup
    free_config(config);
    printf("\rDIRETTORE: Goodbye!\n");
    return 0;
}

static char *parse_args(int argc, char **args) {
    int i = 1;
    while(i < argc && strcmp(args[i], ARG_CONFIG_FILE) != 0) {
        i++;
    }
    i++;
    return i < argc ? args[i]:(char *)DEFAULT_CONFIG_FILE;
}

static int fork_store(char *config_file, pid_t *pid) {
    *pid = fork();
    switch (*pid) {
        case 0:
            //corrisponde ad esempio a ./bin/supermercato -c config.txt
            execl(STORE_EXECUTABLE_PATH, STORE_EXECUTABLE_NAME, config_file);
            //Se exec ritorna allora c'è stato sicuramente errore ed errno è stato impostato
            return -1;
        case -1:
            return -1;
        default: //il padre, quindi il direttore, prosegue
            break;
    }

    return 0;
}

