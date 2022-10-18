/**
 * MULTIFLOW DEVICE DRIVER
 * 
 * Author: Matteo Chiacchia (0300177)
 * SOA project
 * 
 * 
 * 
 * Il progetto permette la creazione e l'installazione di un Linux Device Driver 
 * che permette di eseguire operazioni di lettura e scrittura. Questo implementa flussi di 
 * dati ad alta e bassa priorità in cui è possibile specificare il tipo di operazioni
 * da effettuare. I dati vengono letti seguendo un ordine FIFO e una volta consumati vengono 
 * eliminati dal flusso. Questo driver dà anche il supporto alla funzione ioctl() per permettere 
 * all'utente di cambiare il tipo di sessione utilizzata (livello di priorità e tipo di operazioni, 
 * bloccanti o non bloccanti).
 * 
 * 
 * */




#include "structs.h"
#include "values.h"
#include "read_write_functions.h"
#include "lock_functions.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Matteo Chiacchia");
MODULE_DESCRIPTION("Multi-flow driver file");


#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0)
#define get_major(session)	MAJOR(session->f_inode->i_rdev)
#define get_minor(session)	MINOR(session->f_inode->i_rdev)
#else
#define get_major(session)	MAJOR(session->f_dentry->d_inode->i_rdev)
#define get_minor(session)	MINOR(session->f_dentry->d_inode->i_rdev)
#endif




static int Major; //major number del device driver
static int Minor; //minor number del device file



static int dev_open(struct inode *inode, struct file *file);
static int dev_release(struct inode *inode, struct file *file);
static ssize_t dev_read(struct file *filp, char *buff, size_t len, loff_t *off);
static ssize_t dev_write(struct file *filp, const char *buff, size_t len, loff_t *off);
static long dev_ioctl(struct file *, unsigned int, unsigned long);


/**
 *  File operations del driver.
 * 
 * */
static struct file_operations fops = {
        .owner = THIS_MODULE,
        .write = dev_write,
        .read = dev_read,
        .open =  dev_open,
        .release = dev_release,
        .unlocked_ioctl = dev_ioctl

};


/**
 * Ioctl qui è utilizzata per la gestione della sessione di I/O
 * Permette la modfica di
 * - priority
 * - blocking
 * - timeout
 * 
 * @filp: puntatore a struct file
 * @command: ioctl command
 * @param: valore relativo a command
 * 
 * 
 * Returns: 0
 * 
 * 
 * */
static long dev_ioctl(struct file *filp, unsigned int command, unsigned long param) {

    Session *session = filp->private_data;
    printk("%s: User decided %d command\n", MODNAME, command);
    switch(command){


        case CHANGE_PRIORITY_IOCTL:

            
            if (param == LOW_PRIORITY)
            {
                printk("%s: User decided to change priority to: LOW_PRIORITY\n", MODNAME);
                
            }
            else if (param == HIGH_PRIORITY){
                printk("%s: User decided to change priority to: HIGH_PRIORITY\n", MODNAME);
                
            }
            
            session->priority = param;
            break;
            
            


        case CHANGE_BLOCKING_IOCTL:

            if (param == 0)
            {
                printk("%s: User decided to change blocking to: NON_BLOCKING\n", MODNAME);
                session->timeout=0;
                session->blocking=NON_BLOCKING;
                
            }
            else if (param > 0){
                printk("%s: User decided to change blocking to: BLOCKING\n", MODNAME);
                printk("%s: User decided to change timeout to: %ld\n", MODNAME, param);
                session->timeout=param;
                session->blocking=BLOCKING;
                
            }
            
            break;


      
        default:

            printk("%s: Illegal command by user\n", MODNAME);


    }

    return 0;
}



/**
 * 
 * Funzione per l'apertura del device file
 * 
 * */

static int dev_open(struct inode *inode, struct file *file) {

    Session *session;

    Minor = get_minor(file);

    

    if(Minor >= MINORS || Minor<0){
        return -ENODEV;
    }
    
    // se il device file è stato disattivo non si può aprire una sessione
    if (enabled_device[Minor] == DISABLED) {
        printk("%s: device with minor %d is disabled, and cannot be opened.\n", MODNAME, Minor);
        return -ENOMEM;
    }

    session = kzalloc(sizeof(session), GFP_ATOMIC);
    printk("%s: ALLOCATED new session\n", MODNAME);
    if (session == NULL)
    {
        printk("%s: unable to allocate new session\n", MODNAME);
        return -ENOMEM;
    }
    
    
    // sessione di default
    session->priority = HIGH_PRIORITY;
    session->blocking = NON_BLOCKING;
    session->timeout = 0;
    file->private_data = session;



    printk("%s: device file successfully opened for object with minor %d\n", MODNAME, Minor);
    return 0;

}

/**
 * 
 * Funzione per la chiusura del device file
 * 
 * */
static int dev_release(struct inode *inode, struct file *file){


    Session *session = file->private_data;
    kfree(session);

    printk("%s: driver file closed\n", MODNAME);

    return 0;
}


/**
 * 
 * Funzione per la scrittura sul device file
 * 
 * */

static ssize_t dev_write(struct file *filp, const char *buff, size_t len, loff_t *off) {


    size_t new_bytes;
    
    Object_state* object = &objects[Minor];

    Session *session = filp->private_data;
    Flow *flow = &object->flows[session->priority];


    if (len > object->available_bytes){
        printk("%s: non enough available bytes in driver driver\n", MODNAME);
        return -ENOMEM;
    }

    if (session->priority == HIGH_PRIORITY){

        if (session->blocking == BLOCKING){
            printk("%s: somebody called a BLOCKING HIGH-PRIORITY write on dev with " \
         "[major,minor] number [%d,%d]\n", MODNAME, Major, Minor);
        }
        else{
            printk("%s: somebody called a NON-BLOCKING HIGH-PRIORITY write on dev with " \
         "[major,minor] number [%d,%d]\n", MODNAME, Major, Minor);
        }

        /*write operation*/
        new_bytes = hp_write(object, session, buff, len, Minor);

    }
    else {

        if (session->blocking == BLOCKING){

            printk("%s: somebody called a BLOCKING LOW-PRIORITY write on dev with " \
         "[major,minor] number [%d,%d]\n", MODNAME, Major, Minor);

        }
        else{
            printk("%s: somebody called a NON-BLOCKING LOW-PRIORITY write on dev with " \
         "[major,minor] number [%d,%d]\n", MODNAME, Major, Minor);
        }

        /*mettere in coda la scrittura*/
        new_bytes = write_work_schedule(object, session, buff, len, Minor);


    }

    mutex_unlock(&(flow->operation_synchronizer));
    wake_up(&(flow->wait_queue));
    printk("%s: Lock released.\n", MODNAME);

    return new_bytes;



}



/**
 * 
 * Funzione per la lettura dal device file
 * 
 * */
static ssize_t dev_read(struct file *filp, char *buff, size_t len, loff_t *off){

    Object_state* object = &objects[Minor];
    int bytes_read;
    Session *session = filp->private_data;
    




    if (session->priority == HIGH_PRIORITY){

        if (session->blocking == BLOCKING){
            printk("%s: somebody called a BLOCKING HIGH-PRIORITY read on dev with " \
         "[major,minor] number [%d,%d]\n", MODNAME, Major, Minor);
        }
        else{
            printk("%s: somebody called a NON-BLOCKING HIGH-PRIORITY read on dev with " \
         "[major,minor] number [%d,%d]\n", MODNAME, Major, Minor);
        }

        bytes_read = read_bytes(object, session, buff, len, HIGH_PRIORITY, Minor);

    }
    else {

        if (session->blocking == BLOCKING){

            printk("%s: somebody called a BLOCKING LOW-PRIORITY read on dev with " \
         "[major,minor] number [%d,%d]\n", MODNAME, Major, Minor);

        }
        else{
            printk("%s: somebody called a NON-BLOCKING LOW-PRIORITY read on dev with " \
         "[major,minor] number [%d,%d]\n", MODNAME, Major, Minor);
        }

        bytes_read = read_bytes(object, session, buff, len, LOW_PRIORITY, Minor);


    }



    return bytes_read;
}


/**
 * 
 * Inizializzazione e montaggio del modulo kernel. Si allocano tutte le strutture dati
 * che verranno utilizzate e si registra il modulo nel kernel.
 * 
 * */
int init_module(void){

    int i,j;

    //initialize the drive internal state
    for(i=0;i<MINORS;i++){

        for (j=0; j<FLOWS; j++) {

            mutex_init(&(objects[i].flows[j].operation_synchronizer));

            

            //allocazione di memoria per le operazioni di write
            objects[i].flows[j].obj_head = kzalloc(sizeof(Object_content), GFP_KERNEL);
            if (objects[i].flows[j].obj_head == NULL)
            {
                printk("%s: unable to allocate a new memory node\n", MODNAME);
                goto revert_allocation;
            }

            objects[i].flows[j].obj_head->next=NULL;
            objects[i].flows[j].obj_head->last_offset_read = 0;
            objects[i].flows[j].obj_head->stream_content=NULL;
            
            init_waitqueue_head(&(objects[i].flows[j].wait_queue));


        }
        objects[i].available_bytes = MAX_DEVICE_BYTES;
        enabled_device[i]=ENABLED;


    }

    Major = __register_chrdev(0, 0, 256, DEVICE_NAME, &fops);

    //actually allowed minors are directly controlled within this driver

    if (Major < 0) {
        printk("%s: registering driver failed\n",MODNAME);
        return Major;
    }

    printk("%s: new driver registered, it is assigned major number %d\n",MODNAME, Major);

    return 0;

    revert_allocation:

    for (; i >= 0; i--)
    {
        for (; j >= 0; j--)
        {
            kfree(objects[i].flows[j].obj_head);
        }
    }
    return -ENOMEM;
}


/**
 * 
 * Smontaggio del modulo kernel. Viene liberata la memoria dedicata al modulo
 * e si de-registra il modulo dal kernel
 * 
 * */
void cleanup_module(void)
{

    int i, j;

    Object_content* current_node;
    Object_content* temp;
    for (i = 0; i < MINORS; i++)
    {
        for (j = 0; j < FLOWS; j++)
        {
            current_node = objects[i].flows[j].obj_head;
            while (current_node!=NULL){
                temp = current_node->next;
                kfree(current_node->stream_content);
                kfree(current_node);
                current_node = temp;
            }
        }
    }

    unregister_chrdev(Major, DEVICE_NAME);

    printk("%s: new driver unregistered, it was assigned major number %d\n", MODNAME, Major);

}


