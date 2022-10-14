//
// Created by chacchius on 05/10/22.
//

#ifndef SOAPROJECT_READ_WRITE_FUNCTIONS_H
#define SOAPROJECT_READ_WRITE_FUNCTIONS_H

#include "values.h"
#include "structs.h"
#include "lock_functions.h"

size_t hp_write(Object_state *, Session *, const char *, size_t, int);
size_t write_work_schedule(Object_state *object, Session *session, const char *buff, size_t len, int minor);
void delayed_write(struct work_struct *delayed_work);
int read(Object_state *object, Session *session, char* buff, size_t len, int priority, int Minor);



size_t hp_write(Object_state *object, Session *session, const char *buff, size_t len , int minor){


    char *buffer;
    int lock;
    size_t ret;
    Flow *actual_flow = &object->flows[HIGH_PRIORITY];

    Object_content *current_node= actual_flow->obj_head;
    Object_content *new_content;

    lock = try_lock(object, session, minor);
    printk("%s: User write %d bytes\n", MODNAME, len);

    if (lock==0){
        printk("%s: Cannot acquire the lock, operation failed\n", MODNAME);
        return -1;
    }

    printk("%s: User write %d bytes\n", MODNAME, len);
   
    buffer = kzalloc(len + 1, GFP_ATOMIC);
    new_content = kzalloc(sizeof(Object_content), GFP_ATOMIC);

    if (buffer==NULL || new_content == NULL) {
        printk("%s: Memory allocation error.\n", MODNAME);
        return -ENOMEM;
    }

    ret = copy_from_user(buffer, buff, len);


    

    while (current_node->next != NULL)
        current_node = current_node->next;

    current_node->stream_content = buffer;
    current_node->next = new_content;


    new_content->next = NULL;
    new_content->last_offset_read = 0;
    new_content->stream_content = NULL;

    object->available_bytes -= (len-ret);
    hp_bytes[minor]+=(len - ret);
    return len - ret;

}



size_t write_work_schedule(Object_state *object, Session *session, const char *buff, size_t len, int minor) {
    int ret;
    int lock;
    packed_work_struct *packed_work;
    
    lock = try_lock(object, session, minor);

    if (lock==0){
        printk("%s: Cannot acquire the lock, operation failed\n", MODNAME);
        return -1;
    }


    printk("%s: deferred work init.\n", MODNAME);

    packed_work = kzalloc(sizeof(packed_work_struct), GFP_ATOMIC);
    if (packed_work == NULL) {
        printk("%s: packed_work allocation failure\n", MODNAME);
        return -1;
    }

    // Allocazione delle strutture per effettuare la scrittura successivamente
    packed_work->data = kzalloc(len + 1, GFP_ATOMIC);
    if (packed_work->data == NULL) {
        printk("%s: packed_work data allocation failure\n", MODNAME);
        return -1;
    }

    packed_work->new_content = kzalloc(sizeof(Object_content), GFP_ATOMIC);
    if (packed_work->new_content == NULL) {
        printk("%s: packed_work new_content allocation failure\n", MODNAME);
        return -1;
    }

    ret = copy_from_user((char *)packed_work->data, buff, len);
    packed_work->minor = minor;
    object->available_bytes -= (len - ret);
    lp_bytes[minor]+= (len - ret);

    printk("%s: work buffer allocation success\n", MODNAME);

    __INIT_WORK(&(packed_work->work), &delayed_write, (unsigned long)&(packed_work->work));
    schedule_work(&packed_work->work);

    return len - ret;
}

void delayed_write(struct work_struct *delayed_work){


    packed_work_struct *packed_work = container_of(delayed_work, packed_work_struct, work);
    int minor = packed_work->minor;
    Object_state *object = &objects[minor];
    Flow *flow = &object->flows[LOW_PRIORITY];

    lock(object, minor);

    Object_content *current_node = flow->obj_head;

    while (current_node->next != NULL)
        current_node = current_node->next;

    current_node->stream_content = (char*)packed_work->data;

    Object_content *new_block = packed_work->new_content;
    new_block->next = NULL;
    new_block->stream_content = NULL;
    new_block->last_offset_read = 0;
    current_node->next = new_block;
    flow->obj_head = current_node;
    printk("%s: Written '%s' in block\n", MODNAME, current_node->stream_content) ;

    kfree(packed_work);
    mutex_unlock(&(flow->operation_synchronizer));
    wake_up(&(flow->wait_queue));
    printk("%s: Lock released.\n", MODNAME);


}

int read(Object_state *object, Session *session, char* buff, size_t len, int priority, int Minor) {

    int content_len;
    int ret;
    Object_content *content_to_remove;
    //si effettua un trylock lock
    int lock = try_lock(object, session, Minor);
    Flow *flow = &object->flows[priority];
    printk("%s: priority: %d\n", MODNAME, priority);
    int cl_usr= clear_user(buff, len);

    if (lock==0){
        printk("%s: cannot acquire the lock\n", MODNAME);
        return -1;
    }

    Object_content *current_node = flow->obj_head;
    printk("%s: start reading from first block \n", MODNAME);

    // Non sono presenti dati nello stream, quindi viene rilasciato il lock e si ritorna al chiamante.
    if (current_node->stream_content == NULL) {
        printk("%s: no data in driver, %d\n", MODNAME, priority);
        mutex_unlock(&(flow->operation_synchronizer));
        wake_up(&(flow->wait_queue));
        printk("%s: lock released\n", MODNAME);
        return -1;
    }

    int bytes_read = 0;
    int bytes_to_read = len;

    //lettura dei bytes richiesti
    while (1){

        content_len = strlen(current_node->stream_content);


        //caso in cui il numero di bytes da leggere è minore del numero di bytes nel current_block
        if (bytes_to_read <= content_len - current_node->last_offset_read){
            printk("%s: can read only from one node: %d total bytes to read\n", MODNAME, bytes_to_read);

            ret = copy_to_user(&buff[bytes_read], &current_node->stream_content[current_node->last_offset_read], bytes_to_read);
            current_node->last_offset_read += (bytes_to_read-ret);

            bytes_read += (bytes_to_read-ret);

            if(priority==HIGH_PRIORITY) hp_bytes[Minor] -= bytes_read;
            else lp_bytes[Minor] -= bytes_read;

            object->available_bytes += bytes_read;

            if (current_node->last_offset_read > content_len - 1){
                //elimino il blocco se tutti i bytes presenti sono stati consumati
                content_to_remove = current_node;
                current_node = current_node->next;
                flow->obj_head = current_node;

                kfree(content_to_remove->stream_content);
                kfree(content_to_remove);
                printk("%s: Read | Block fully read. Memory released.", MODNAME);
            }

            //rilascio il lock e finisco l'iterazione
            mutex_unlock(&(flow->operation_synchronizer));
            wake_up(&(flow->wait_queue));
            printk("%s: Read completed and lock released.\n", MODNAME);
            break;



        }


        //caso in cui il numero di bytes da leggere è maggiore del numero di bytes nel current_block
        else{

            printk("%s: should be read from more than one node: %d total bytes to read\n", MODNAME, bytes_to_read);

            int residual_bytes = content_len - current_node->last_offset_read;

            ret = copy_to_user(&buff[bytes_read], &current_node->stream_content[current_node->last_offset_read], residual_bytes);

            bytes_to_read -= (residual_bytes-ret);
            bytes_read += (residual_bytes-ret);

            content_to_remove = current_node;
            current_node = current_node->next;
            flow->obj_head = current_node;

            kfree(content_to_remove->stream_content);
            kfree(content_to_remove);

            printk("%s: Read | Block fully read. Memory released.", MODNAME);

            // non sono più presenti byte nel driver
            if (current_node->stream_content == NULL){

                if(priority==HIGH_PRIORITY) hp_bytes[Minor] -= bytes_read;
                else lp_bytes[Minor] -= bytes_read;
                object->available_bytes += bytes_read;
                printk("%s: Read | Block fully read. Memory released. There aren't other bytes in driver to read\n", MODNAME);
                //rilascio il lock e finisco l'iterazione
                mutex_unlock(&(flow->operation_synchronizer));
                wake_up(&(flow->wait_queue));
                printk("%s: Read completed and lock released.\n", MODNAME);
                break;
            }



        }



    }

    return bytes_read;


}


#endif //SOAPROJECT_READ_WRITE_FUNCTIONS_H
