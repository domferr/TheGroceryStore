#define _POSIX_C_SOURCE 200809L

#include "signal_handler.h"
#include "grocerystore.h"
#include "cashier.h"
#include "utils.h"
#include "queue.h"
#include "scfiles.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

//#define DEBUGSIGHANDLER

static int wakeup_cashier(cashier_t *ca);

//Funzione del thread handler
void *thread_handler_fun(void *arg) {
    int err, sig;
    size_t i;
    gs_state closing_state;
    signal_handler_t *sh = (signal_handler_t*) arg;
    sigset_t set = sh->set;
    grocerystore_t *gs = sh->gs;
    //Aspetta sul set che arrivi un segnale
    //Se l'attesa non è andata a buon fine
    NOTZERO(sigwait(&set, &sig), perror("sigwait"); free(arg); return NULL;)
    //Gestione del segnale arrivato
    switch (sig) {
        case SIGINT:
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
#ifdef DEBUGSIGHANDLER
    printf("Signal handler: Chiudo il supermercato\n");
#endif
    //Imposto lo stato del supermercato, non faccio entrare più clienti
    PTH(err, pthread_mutex_lock(&(gs->mutex)), return 0)
    gs->state = closing_state;
    gs->can_enter = 0;
    PTH(err, pthread_cond_broadcast(&(gs->entrance)), return 0) //Sveglio tutti i thread che aspettano di entrare
    PTH(err, pthread_cond_signal(&(gs->exit)), return 0)   //Sveglio il gestore delle entrate
    PTH(err, pthread_mutex_unlock(&(gs->mutex)), return 0)
#ifdef DEBUGSIGHANDLER
    printf("Signal handler: Ho impostato lo stato del supermercato in chiusura\n");
#endif
    //Cambio lo stato di ogni cassa per segnalare che il supermercato è in chiusura
    for(i=0; i<(sh->no_of_cashiers); ++i) {
        err = wakeup_cashier((sh->cashiers)[i]);
        NOTZERO(err, perror("wakeup cashier"); return 0)
    }

#ifdef DEBUGSIGHANDLER
    printf("Signal handler: Termino\n");
#endif
    free(arg);
    return 0;
}

static int wakeup_cashier(cashier_t *ca) {
    int err;
    queue_t *queue = ca->queue;
    PTH(err, pthread_mutex_lock(&(ca->mutex)), return 0)
    //Sveglio il cassiere se dovesse essere una cassa chiusa
    if (ca->state == sleeping) {
        PTH(err, pthread_cond_signal(&(ca->paused)), return 0)
    } else if (ca->state == active && queue->size == 0) {
    //Sveglio il cassiere se dovesse essere in attesa sulla coda vuota
        PTH(err, pthread_cond_signal(&(ca->noclients)), return 0)
    }
    PTH(err, pthread_mutex_unlock(&(ca->mutex)), return 0)
    return 0;
}
