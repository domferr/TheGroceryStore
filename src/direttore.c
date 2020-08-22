#define DEBUGGING 0
#include "../include/sig_handling.h"
#include "../libs/utils/include/utils.h"
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

#define MESSAGE_VALID_CONFIG_FILE "File di configurazione: "

#define CASSA_APERTA 0
#define CASSA_CHIUSA (-1)
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

/**
 * Gestisce l'arrivo di una notifica dal processo supermercato. Ritorna 0 in caso di successo, -1 altrimenti ed imposta
 * errno.
 *
 * @param fd_store descrittore del file utilizzato per la comunicazione socket AF_UNIX con il supermercato
 * @param config struttura di configurazione
 * @param casse array nel quale l'elemento in posizione i corrisponde al numero di clienti presenti nella cassa i
 * @param casse_attive contatore di quante casse sono attive
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno
 */
static int handle_notification(int fd_store, config_t *config, int *casse, int *casse_attive);

/**
 * Gestisce la richiesta di uscita di un cliente. Il permesso viene sempre concesso. Ritorna 0 in caso di successo,
 * altrimenti -1 ed imposta errno.
 *
 * @param fd_store descrittore del file utilizzato per la comunicazione socket AF_UNIX con il supermercato
 * @param client_id identificatore univoco del cliente che ha richiesto l'uscita
 * @return 0 in caso di successo, -1 altrimenti ed imposta errno
 */
static int handle_ask_exit(int fd_store, int client_id);

int main(int argc, char **args) {
    int err, i, param2, fd_store, sig_arrived, sigh_pipe[2], *casse, casse_attive;
    pid_t pid_store;                //pid del processo supermercato
    config_t *config;               //Struttura con i parametri di configurazione
    fd_set set, rd_set;
    msg_header_t msg_hdr;

    //Eseguo il parsing del nome del file di configurazione
    char *config_file_path = parse_args(argc, args);
    //Leggo il file di configurazione
    EQNULLERR(config = load_config(config_file_path), exit(EXIT_FAILURE))
    if (!validate(config)) {
        free_config(config);
        exit(EXIT_FAILURE);
    }
    printf(MESSAGE_VALID_CONFIG_FILE);
    print_config(config);
    EQNULLERR(casse = malloc(sizeof(int) * config->k), exit(EXIT_FAILURE))
    casse_attive = config->ka;
    for (i = 0; i < config->k; i++) {
        casse[i] = i < config->ka ? CASSA_APERTA:CASSA_CHIUSA;   //le prime ka casse sono aperte
    }
    //Lancio il processo supermercato
    MINUS1ERR(fork_store(config_file_path, &pid_store), exit(EXIT_FAILURE))
    DEBUG("DIRETTORE: %s\n", "Connesso con il supermercato via socket AF_UNIX");
    //Gestione dei segnali mediante thread apposito
    MINUS1ERR(pipe(sigh_pipe), exit(EXIT_FAILURE))
    MINUS1ERR(handle_signals(&thread_sig_handler_fun, (void*)sigh_pipe), exit(EXIT_FAILURE))
    //Creo una connessione con il supermercato via socket AF_UNIX
    MINUS1ERR(fd_store = accept_socket_conn(), exit(EXIT_FAILURE))

    FD_ZERO(&set);  //azzero il set
    FD_SET(fd_store, &set); //imposto il descrittore per comunicare con il supermercato via socket AF_UNIX
    FD_SET(sigh_pipe[0], &set); //imposto il descrittore per comunicare via pipe con il thread signal handler

    //Esco solo se ricevo qualcosa dal thread sig handler via pipe oppure se il la comunicazione via socket viene meno
    //a seguito di una terminazione imprevista del processo supermercato
    while (1) {
        rd_set = set;
        MINUS1ERR(select(fd_store+1, &rd_set, NULL, NULL, NULL), exit(EXIT_FAILURE))
        if (FD_ISSET(sigh_pipe[0], &rd_set)) {
            MINUS1ERR(readn(sigh_pipe[0], &sig_arrived, sizeof(int)), exit(EXIT_FAILURE))
            MINUS1ERR(kill(pid_store, sig_arrived), exit(EXIT_FAILURE))
            break;
        } else {
            MINUS1ERR(err = readn(fd_store, &msg_hdr, sizeof(msg_header_t)), exit(EXIT_FAILURE))
            if (err == 0) { //EOF quindi il processo supermercato è terminato in maniera imprevista!
                break;
            } else {    //altrimenti gestisco il messaggio ricevuto
                switch (msg_hdr) {
                    case head_notify: //Ricevuta notifica. Eseguo algoritmo di apertura/chiusura cassa ed eventualmente apro/chiudo una cassa
                        //leggo idcassa e numero di clienti in coda
                        MINUS1ERR(readn(fd_store, &i, sizeof(int)), exit(EXIT_FAILURE))
                        MINUS1ERR(readn(fd_store, &param2, sizeof(int)), exit(EXIT_FAILURE))
                        DEBUG("Ho ricevuto che nella cassa %d ci sono %d clienti in coda\n", i, param2);
                        //Potrei aver chiuso la cassa mentre ricevevo questa notifica quindi aggiorno solo se la cassa è aperta
                        if (casse[i] != CASSA_CHIUSA) {
                            casse[i] = param2;
                            MINUS1ERR(handle_notification(fd_store, config, casse, &casse_attive), exit(EXIT_FAILURE))
                        }
                        break;
                    case head_ask_exit: //Ricevuta richiesta per uscire dal supermercato. Dò sempre il permesso
                        //leggo l'id del cliente che vuole uscire
                        MINUS1ERR(readn(fd_store, &i, sizeof(int)), exit(EXIT_FAILURE))
                        DEBUG("%s\n","Ho ricevuto una richiesta di uscita da parte di un cliente");
                        //rispondo facendo uscire il cliente
                        MINUS1ERR(handle_ask_exit(fd_store, i), exit(EXIT_FAILURE))
                        break;
                    default:
                        break;
                }
            }
        }
    }
    //chiudo i file descriptors
    close(sigh_pipe[0]);
    close(sigh_pipe[1]);
    //attendo il processo supermercato
    MINUS1ERR(waitpid(pid_store, &err, 0), exit(EXIT_FAILURE))
    close(fd_store);
    //memory cleanup
    free_config(config);
    free(casse);
    DEBUG("\r%s\n", "DIRETTORE: Goodbye!");
    return 0;
}

static int handle_notification(int fd_store, config_t *config, int *casse, int *casse_attive) {
    int i, count_attive = *casse_attive, count_nonattive = config->k - *casse_attive, attive = *casse_attive,
        count_s1 = 0, count_s2 = 0,
        cassa_s1 = -1, cassa_s2 = -1;
    unsigned int seed = getpid();
    msg_header_t msg_hdr;
    for (i = 0; i < config->k; i++) {
        if (casse[i] == CASSA_CHIUSA) {    //Se la cassa è chiusa
            count_nonattive--;
            //Se non ho ancora scelto una cassa ne scelgo una casualmente con probabilità del 50%.
            //Se alla fine non ho scelto la cassa, allora scelgo l'ultima tra quelle chiuse
            if (cassa_s2 == -1 && ((count_nonattive == 0) || probability(seed, 50)))
                cassa_s2 = i;
        } else if (casse[i] >= 0) {    //Se la cassa è aperta
            count_attive--;
            //Se non ho ancora scelto una cassa ne scelgo una casualmente con probabilità del 50%.
            //Se alla fine non ho scelto la cassa, allora scelgo l'ultima tra quelle aperte
            if (cassa_s1 == -1 && ((count_attive == 0) || probability(seed, 50)))
                cassa_s1 = i;
        }

        if (casse[i] == 0 || casse[i] == 1) count_s1++;
        if (casse[i] >= config->s2) count_s2++;
    }

    //Se è aperta più di una cassa e sono nella soglia, allora posso chiudere una cassa
    if (attive > 1 && count_s1 >= config->s1) {
        msg_hdr = head_close;
        MINUS1(writen(fd_store, &msg_hdr, sizeof(msg_header_t)), return -1)
        MINUS1(writen(fd_store, &cassa_s1, sizeof(int)), return -1)
        *casse_attive = *casse_attive - 1;
        casse[cassa_s1] = CASSA_CHIUSA;
        DEBUG("Chiudo la cassa %d\n", cassa_s1)
    }
    //Se sono aperte meno di k casse e sono nella soglia, allora posso aprire un'altra cassa
    if (attive < config->k && count_s2 > 0) {
        msg_hdr = head_open;
        MINUS1(writen(fd_store, &msg_hdr, sizeof(msg_header_t)), return -1)
        MINUS1(writen(fd_store, &cassa_s2, sizeof(int)), return -1)
        *casse_attive = *casse_attive + 1;
        casse[cassa_s2] = CASSA_APERTA;
        DEBUG("Apro la cassa %d\n", cassa_s2)
    }
    return 0;
}

static int handle_ask_exit(int fd_store, int client_id) {
    int msg_param = 1;  //Concedi sempre il permesso
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
            execl(STORE_EXECUTABLE_PATH, STORE_EXECUTABLE_NAME, config_file, (char*) 0);
            //Se exec ritorna allora c'è stato sicuramente errore ed errno è stato impostato
            return -1;
        case -1:
            return -1;
        default: //il padre, quindi il direttore, prosegue
            break;
    }

    return 0;
}