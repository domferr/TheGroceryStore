#define _POSIX_C_SOURCE 200809L
#include "signal_handler.h"
#include "grocerystore.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

//#define DEBUGSIGHANDLER

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
#ifdef DEBUGSIGHANDLER
                printf("\rArrivato SIGINT\n");
#endif
                break;
            case SIGQUIT:
#ifdef DEBUGSIGHANDLER
                printf("\rDa questo momento il supermercato è chiuso e i clienti all'interno verranno fatti uscire immediatamente\n");
#endif
                //free((signal_handler_t*)arg);
                //return NULL;
                break;
            case SIGHUP:
#ifdef DEBUGSIGHANDLER
                printf("\rDa questo momento i clienti non verranno più fatti entrare nel supermercato\n");
#endif
                break;
            default:
                break;
        }
    //}
    pthread_mutex_lock(&(gs->mutex));
    gs->state = closed;
    gs->can_enter = 0;
    pthread_cond_broadcast(&(gs->entrance));    //Sveglio tutti i thread che aspettano di entrare
    pthread_cond_signal(&(gs->exit));   //Sveglio il gestore delle entrate
    pthread_mutex_unlock(&(gs->mutex));
    free(arg);
#ifdef DEBUGSIGHANDLER
    printf("Signal handler: Termino\n");
#endif
    return 0;
}
