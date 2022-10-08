//
// Created by chacchius on 03/10/22.
//

#ifndef SOAPROJECT_STRUCTS_H
#define SOAPROJECT_STRUCTS_H

#include "values.h"


/**
 *  Struttura rappresentante lo stream salvato
 */
typedef struct _object_content{

    int last_offset_read;                       //ultimo offset letto
    char *stream_content;                       //bytes scritti
    struct _object_content *next;               //puntatore al blocco successivo

} Object_content;


/**
 *  Struttura per la gestione del flusso (bassa o alta priorità)
 */
typedef struct _flow{

    struct mutex operation_synchronizer;        //lock di gestione dei thread concorrenti
    Object_content *obj_head;                   //inizio della lista collegata in cui sono presenti i blocchi di bytes scritti
    wait_queue_head_t *waitQueueHead;           //wait queue in cui sono presenti i task in sleep

} Flow;


/**
 *  Struttura per la gestione del device
 */
typedef struct _object_state{

    long available_bytes;                  //numero di bytes che il device può ancora gestire
    Flow flows[FLOWS];                    //flussi (bassa e alta priority) gestiti dal device

}Object_state;

/**
 *  Struttura per la gestione della sessione
 */
typedef struct _session{

    int priority;               //priorità delle operazioni (alta o bassa)
    int blocking;               //operazioni (write or read) bloccanti o non bloccanti
    unsigned long timeout;       //impostazione di un timeout che regola l'attivazione delle blocking operations

}Session;


/**
 *  Struttura per il deferred work
 */
typedef struct _packed_work_struct {
    const char *data;                   // Puntatore al buffer kernel temporaneo dove sono salvati i dati da scrivere poi sullo stream.
    size_t len;                         // Quantità di dati da scrivere, corrisponde alla lunghezza del buffer 'data'.
    Object_content *new_content;        // Puntatore al blocco vuoto per la scrittura successiva.
    int minor;                          // Minor number del device su cui si sta operando.
    Session *session;                   // Puntatore alla session_state verso il device su cui effettuare la scrittura.
    struct work_struct work;            // Struttura di deferred work
} packed_work_struct;




Object_state objects[MINORS];




#endif //SOAPROJECT_STRUCTS_H
