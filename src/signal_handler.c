#define _POSIX_C_SOURCE 200809L
#include "signal_handler.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

//Funzione del thread handler
void *signal_handler_fun(void *arg) {
    sigset_t set = ((signal_handler_t*) arg)->set;
    int sig;

    while (1) {
        printf("ASPETTO SEGNALI, DIOCANE\n");
        //Aspetta sul set che arrivi un segnale
        if (sigwait(&set, &sig) != 0 ) {  //Se l'attesa non è andata a buon fine
            perror("sigwait");
            free(arg);
            return NULL;
        }
        //Gestione del segnale arrivato
        switch (sig) {
            case SIGINT:
                printf("\rArrivato SIGINT\n");
                break;
            case SIGQUIT:
                printf("\rArrivato SIGQUIT\n");
                //free((signal_handler_t*)arg);
                //return NULL;
                break;
            case SIGHUP:
                break;
            default:
                break;
        }
    }

    free(arg);
    return 0;
}
