#define _POSIX_C_SOURCE 200809L
#include "signal_handler.h"
#include "grocerystore.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

//Funzione del thread handler
void *thread_handler_fun(void *arg) {
    sigset_t set = ((signal_handler_t*) arg)->set;
    grocerystore_t *gs = ((signal_handler_t*) arg)->gs;
    int sig;

    //while (1) {
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
                printf("\rDa questo momento il supermercato è chiuso e i clienti all'interno verranno fatti uscire immediatamente\n");
                //free((signal_handler_t*)arg);
                //return NULL;
                break;
            case SIGHUP:
                printf("\rDa questo momento i clienti non verranno più fatti entrare nel supermercato\n");
                break;
            default:
                break;
        }
    //}
    pthread_mutex_lock(&(gs->mutex));
    gs->state = closed;
    gs->can_enter = 0;
    pthread_cond_broadcast(&(gs->entrance));
    pthread_cond_signal(&(gs->exit));
    pthread_mutex_unlock(&(gs->mutex));
    free(arg);
    printf("Signal handler: Termino\n");
    return 0;
}
