#define _POSIX_C_SOURCE 200809L
#include "signal_handler.h"
#include "grocerystore.h"
#include "utils.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

//#define DEBUGSIGHANDLER

//Funzione del thread handler
void *thread_handler_fun(void *arg) {
    int err;
    sigset_t set = ((signal_handler_t*) arg)->set;
    grocerystore_t *gs = ((signal_handler_t*) arg)->gs;
    int sig;

    //Aspetta sul set che arrivi un segnale
    //Se l'attesa non è andata a buon fine
    NOTZERO(sigwait(&set, &sig), perror("sigwait"); free(arg); return NULL;)
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
    PTH(err, pthread_mutex_lock(&(gs->mutex)), return 0)
    gs->state = closed;
    gs->can_enter = 0;
    PTH(err, pthread_cond_broadcast(&(gs->entrance)), return 0) //Sveglio tutti i thread che aspettano di entrare
    PTH(err, pthread_cond_signal(&(gs->exit)), return 0)   //Sveglio il gestore delle entrate
    PTH(err, pthread_mutex_unlock(&(gs->mutex)), return 0)
    free(arg);
#ifdef DEBUGSIGHANDLER
    printf("Signal handler: Termino\n");
#endif
    return 0;
}
