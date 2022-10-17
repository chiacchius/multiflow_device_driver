/**
 * 
 * Header per la gestione delle funzioni di lock.
 * Contiene le funzioni di appoggio per il corretto funzionamento
 * delle operazioni bloccanti e non.
 * 
 * Author: Matteo Chiacchia (0300177) 
 * 
 * */



#ifndef SOAPROJECT_LOCK_FUNCTIONS_H
#define SOAPROJECT_LOCK_FUNCTIONS_H

#include "values.h"
#include "structs.h"



int try_lock(Object_state *object, Session *session, int minor);
int add_in_waitqueue(unsigned long timeout, struct mutex *synchronizer, wait_queue_head_t *wait_queue);
int lock(Object_state *object, int minor);




/**
 * Funzione per la gestione del mutex_trylock.
 * Se l'operazione è non bloccante e mutex_trylock fallisce allora si ritorna errore.
 * Altrimenti, se l'operazione è bloccante e mutex_trylock fallisce, si aspetta il tempo di timeout
 * in cui si tenta di prendere il lock, aggiungendo il thread nella wait_queue.
 * 
 * @object: indirizzo della struct di gestione del device file 
 * @session: indirizzo della struct rappresentante la sessione attuale
 * @minor: minor number del device file
 *
 * Returns: 0 in caso di fallimento, 1 in caso di successo
 * */
int try_lock(Object_state *object, Session *session, int minor) {

    Flow *actual_flow = &object->flows[session->priority];
    int lock = mutex_trylock(&(actual_flow->operation_synchronizer));
    int ret=0;

    // se il lock non è stato acquisito si gestisce il caso bloccante
    if (lock==0){

        printk("%s: lock not available\n", MODNAME);
        if (session->blocking==BLOCKING){

            printk("%s: blocking operation, until the timeout expires attempt to get lock\n", MODNAME);
            if (session->priority == HIGH_PRIORITY){

                hp_threads[minor]++;
                ret = add_in_waitqueue(session->timeout, &actual_flow->operation_synchronizer, &actual_flow->wait_queue);
                hp_threads[minor]--;

            }
            else{

                lp_threads[minor]++;
                ret = add_in_waitqueue(session->timeout, &actual_flow->operation_synchronizer, &actual_flow->wait_queue);
                lp_threads[minor]--;

            }


            if (ret==0){
                printk( "%s: timeout end, lock not acquired \n", MODNAME);
                return 0;
            }
            else {
                printk("%s: Lock acquired before timeout end\n", MODNAME);
                return 1;
            }


        }



        printk("%s: non-blocking operation, impossible to write data\n", MODNAME);
        return 0;



    }
    printk("%s: Lock acquired\n", MODNAME);
    return 1;
}

/**
 * Funzione che mette il thread nella wait_queue e si mette in attesa del timeout
 * 
 * @timeout: valore di timeout della sessione
 * @synchronizer: indirizzo del mutex che si tenta di lockare
 * @wait_queue: indirizzo della wait_queue in cui il thread viene aggiunto
 * 
 * Returns: 0 lock non acquisito, 1 lock acquisito
 * */
int add_in_waitqueue(unsigned long timeout, struct mutex *synchronizer, wait_queue_head_t *wait_queue) {

    if (timeout<=0){
        return 0;
    }


    printk("%s: process in wait, try to acquire the lock until the timeout expires\n", MODNAME, current->pid);

    //se si riesce a prendere il lock prima della scadenza del timeout allora ret=1, altrimenti ret =0
    return wait_event_timeout(*wait_queue, mutex_trylock(synchronizer), msecs_to_jiffies(timeout));
}





/**
 * Funzione che mette il thread in attesa di prendere ill lock nel caso di 
 * deferred work. Non potendo fallire l'attesa continua fino al momento in cui il thread
 * non riesce ad acquisire il lock
 * 
 * @object: indirizzo della struct di gestione del device file 
 * @minor: minor number del device file
 * 
 * Returns: 1
 * */
int lock(Object_state *object, int minor) {

    Flow *flow = &object->flows[LOW_PRIORITY];

    //una scrittura low priority non può fallire, quindi attendo di prendere il lock
    __sync_fetch_and_add(&lp_threads[minor], 1);
    mutex_lock(&(flow->operation_synchronizer));
    __sync_fetch_and_add(&lp_threads[minor], -1);
    printk("%s: Process %d acquired lock.\n", MODNAME, current->pid);
    return 1;
}




#endif //SOAPROJECT_LOCK_FUNCTIONS_H
