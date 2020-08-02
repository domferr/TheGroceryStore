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
#include <wait.h>

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

static int handle_notification(int fd_store, config_t *config, int *casse, int *casse_attive);

static int handle_ask_exit(int fd_store, int client_id);

int main(int argc, char **args) {
    int err, i, param2, fd_store, sig_arrived, sigh_pipe[2], *casse, casse_attive;
    pid_t pid_store;                //pid del processo supermercato
    pthread_t sig_handler_thread;   //thread che si occupa di gestire i segnali
    config_t *config;               //Struttura con i parametri di configurazione
    fd_set set, rd_set;
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
    EQNULL(casse = malloc(sizeof(int) * config->k), perror("calloc"); exit(EXIT_FAILURE))
    for (i = 0; i < config->k; ++i) {
        casse[i] = i < config->ka ? 0:-1;   //le prime ka casse sono aperte
    }
    casse_attive = config->ka;
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

    //Esco solo se ricevo qualcosa dal thread sig handler via pipe oppure se il la comunicazione via socket viene meno
    //a seguito di una terminazione imprevista del processo supermercato
    while (1) {
        rd_set = set;
        MINUS1(select(fd_store+1, &rd_set, NULL, NULL, NULL), perror("select"); exit(EXIT_FAILURE))
        if (FD_ISSET(fd_store, &rd_set)) {
            printf("Comunicazione dal supermercato\n");
            MINUS1(err = readn(fd_store, &msg_hdr, sizeof(msg_header_t)), perror("readn"); exit(EXIT_FAILURE))
            if (err == 0) { //EOF quindi il processo supermercato è terminato in maniera imprevista! Segnalo il sighandler e termino
                PTH(err, pthread_kill(sig_handler_thread, SIGINT), perror("pthread_kill"); exit(EXIT_FAILURE))
                break;
            } else {    //altrimenti gestisco il messaggio ricevuto
                switch (msg_hdr) {
                    case head_notify: //Ricevuta notifica. Eseguo algoritmo di apertura/chiusura cassa ed eventualmente apro/chiudo una cassa
                        //leggo idcassa e numero di clienti in coda
                        MINUS1(readn(fd_store, &i, sizeof(int)), return -1)
                        MINUS1(readn(fd_store, &param2, sizeof(int)), return -1)
                        printf("Ho ricevuto che nella cassa %d ci sono %d clienti in coda\n", i, param2);
                        casse[i] = param2;
                        MINUS1(handle_notification(fd_store, config, casse, &casse_attive), perror("handle_notification"); exit(EXIT_FAILURE))
                        break;
                    case head_ask_exit: //Ricevuta richiesta per uscire dal supermercato. Dò sempre il permesso
                        //leggo l'id del cliente che vuole uscire
                        MINUS1(readn(fd_store, &i, sizeof(int)), return -1)
                        //rispondo facendo uscire il cliente
                        MINUS1(handle_ask_exit(fd_store, i), perror("handle_ask_exit"); exit(EXIT_FAILURE))
                        break;
                    default:
                        break;
                }
            }
        } else {
            MINUS1(readn(sigh_pipe[0], &sig_arrived, sizeof(int)), perror("readn"); exit(EXIT_FAILURE))
            MINUS1(kill(pid_store, sig_arrived), perror("kill"); exit(EXIT_FAILURE))
            break;
        }
    }

    //join sul thread signal handler
    PTH(err, pthread_join(sig_handler_thread, NULL), perror("join thread handler"); exit(EXIT_FAILURE))
    //chiudo i file descriptors
    close(fd_store);
    close(sigh_pipe[0]);
    close(sigh_pipe[1]);
    //attendo il processo supermercato
    MINUS1(waitpid(pid_store, &err, 0), perror("waitpid"); exit(EXIT_FAILURE))
    //memory cleanup
    free_config(config);
    free(casse);
    printf("\rDIRETTORE: Goodbye!\n");
    return 0;
}

static int handle_notification(int fd_store, config_t *config, int *casse, int *casse_attive) {
    int i, count_attive = 0, count_nonattive = 0,
        count_s1 = 0, count_s2 = 0,
        cassa_s1 = -1, cassa_s2 = -1;
    unsigned int seed = getpid();
    msg_header_t msg_hdr;
    for (i = 0; i < config->k; ++i) {
        if (casse[i] >= 0) {
            count_attive++;
            if (cassa_s1 == -1 && (count_attive == *casse_attive || RANDOM(seed, 0, 2)))
                cassa_s1 = i;
        } else {
            count_nonattive++;
            if (cassa_s2 == -1 && (count_nonattive == config->k - *casse_attive || RANDOM(seed, 0, 2)))
                cassa_s2 = i;
        }
        if (casse[i] == 0 || casse[i] == 1) count_s1++;
        if (casse[i] >= config->s2) count_s2++;
    }

    //Se è aperta più di una cassa e sono nella soglia, allora posso chiudere una cassa
    if (*casse_attive > 1 && count_s1 >= config->s1) {
        msg_hdr = head_close;
        MINUS1(writen(fd_store, &msg_hdr, sizeof(msg_header_t)), return -1)
        MINUS1(writen(fd_store, &cassa_s1, sizeof(int)), return -1)
        *casse_attive = *casse_attive - 1;
    }
    //Se sono aperte meno di k casse e sono nella soglia, allora posso aprire un'altra cassa
    if (*casse_attive < config->k && count_s2 > 0) {
        msg_hdr = head_open;
        MINUS1(writen(fd_store, &msg_hdr, sizeof(msg_header_t)), return -1)
        MINUS1(writen(fd_store, &cassa_s2, sizeof(int)), return -1)
        *casse_attive = *casse_attive + 1;
    }
    return 0;
}

static int handle_ask_exit(int fd_store, int client_id) {
    int msg_param = 1;
    msg_header_t msg_hdr = head_can_exit;
    MINUS1(writen(fd_store, &msg_hdr, sizeof(msg_header_t)), return -1)
    MINUS1(writen(fd_store, &client_id, sizeof(int)), return -1)
    MINUS1(writen(fd_store, &msg_param, sizeof(int)), return -1)
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
