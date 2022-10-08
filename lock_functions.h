//
// Created by chacchius on 05/10/22.
//

#ifndef SOAPROJECT_LOCK_FUNCTIONS_H
#define SOAPROJECT_LOCK_FUNCTIONS_H

#include "values.h"
#include "structs.h"

int try_lock(Object_state *object, Session *session, int minor);
int add_in_waitqueue(unsigned long timeout, struct mutex *synchronizer, wait_queue_head_t *wait_queue);
int lock(Object_state *object, int minor);


int try_lock(Object_state *object, Session *session, int minor) {

    Flow *actual_flow = &object->flows[session->priority];
    int lock = mutex_trylock(&(actual_flow->operation_synchronizer));
    int ret=0;

    if (lock==0){

        printk("%s: lock not available\n", MODNAME);
        if (session->blocking==BLOCKING){

            printk("%s: blocking operation, until the timeout expires attempt to get lock\n", MODNAME);
            if (session->priority == HIGH_PRIORITY){

                hp_threads[minor]++;
                ret = add_in_waitqueue(session->timeout, &actual_flow->operation_synchronizer, actual_flow->waitQueueHead);
                hp_threads[minor]--;

            }
            else{

                lp_threads[minor]++;
                ret = add_in_waitqueue(session->timeout, &actual_flow->operation_synchronizer, actual_flow->waitQueueHead);
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

int add_in_waitqueue(unsigned long timeout, struct mutex *synchronizer, wait_queue_head_t *wait_queue) {

    if (timeout<=0){
        return 0;
    }



    //se si riesce a prendere il lock prima della scadenza del timeout allora ret=1, altrimenti ret =0
    return wait_event_timeout(*wait_queue, mutex_trylock(synchronizer), msecs_to_jiffies(timeout));
}


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
