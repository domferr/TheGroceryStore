#define _POSIX_C_SOURCE 200809L
#include "signal_handler.h"
#include "grocerystore.h"
#include "cashier.h"
#include "utils.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define DEBUGSIGHANDLER

//Funzione del thread handler
void *thread_handler_fun(void *arg) {
    int err, sig;
    size_t i;
    gs_state closing_state;
    sigset_t set = ((signal_handler_t*) arg)->set;
    grocerystore_t *gs = ((signal_handler_t*) arg)->gs;
    cashier_t **cashiers = ((signal_handler_t*) arg)->cashiers;
    size_t no_of_cashiers = ((signal_handler_t*) arg)->no_of_cashiers;
    //Aspetta sul set che arrivi un segnale
    //Se l'attesa non è andata a buon fine
    NOTZERO(sigwait(&set, &sig), perror("sigwait"); free(arg); return NULL;)
    //Gestione del segnale arrivato
    switch (sig) {
        case SIGINT:
#ifdef DEBUGSIGHANDLER
            printf("\rArrivato SIGINT\n");
#endif
            closing_state = closed_fast;
            break;
        case SIGQUIT:
#ifdef DEBUGSIGHANDLER
            printf("\rDa questo momento il supermercato è chiuso e i clienti all'interno verranno fatti uscire immediatamente\n");
#endif
            closing_state = closed_fast;
            break;
        case SIGHUP:
#ifdef DEBUGSIGHANDLER
            printf("\rDa questo momento i clienti non verranno più fatti entrare nel supermercato che chiuderà non appena tutti i clienti escono\n");
#endif
            closing_state = closed;
            break;
        default:
            break;
    }
    //Imposto lo stato del supermercato, non faccio entrare più clienti
    PTH(err, pthread_mutex_lock(&(gs->mutex)), return 0)
    gs->state = closing_state;
    gs->can_enter = 0;
    PTH(err, pthread_cond_broadcast(&(gs->entrance)), return 0) //Sveglio tutti i thread che aspettano di entrare
    PTH(err, pthread_cond_signal(&(gs->exit)), return 0)   //Sveglio il gestore delle entrate
    PTH(err, pthread_mutex_unlock(&(gs->mutex)), return 0)
    //Cambio lo stato di ogni cassa per segnalare che il supermercato è in chiusura
    for(i=0; i<no_of_cashiers; i++) {
        PTH(err, pthread_mutex_lock(&((cashiers[i])->mutex)), return 0)
        cashiers[i]->state = closing;
        PTH(err, pthread_cond_signal(&((cashiers[i])->paused)), return 0)   //Sveglio il cassiere se si tratta di una cassa chiusa
        PTH(err, pthread_mutex_unlock(&((cashiers[i])->mutex)), return 0)
    }
    free(arg);
#ifdef DEBUGSIGHANDLER
    printf("Signal handler: Termino\n");
#endif
    return 0;
}
