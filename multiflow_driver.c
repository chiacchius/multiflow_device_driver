/* Multiflow device driver
 * Author: Matteo Chiacchia
 * SOA project
 * */




#include "structs.h"
#include "values.h"
#include "read_write_functions.h"
#include "lock_functions.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Matteo Chiacchia");
MODULE_DESCRIPTION("Multi-flow device file");


#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0)
#define get_major(session)	MAJOR(session->f_inode->i_rdev)
#define get_minor(session)	MINOR(session->f_inode->i_rdev)
#else
#define get_major(session)	MAJOR(session->f_dentry->d_inode->i_rdev)
#define get_minor(session)	MINOR(session->f_dentry->d_inode->i_rdev)
#endif




static int Major;
static int Minor;

static int dev_open(struct inode *inode, struct file *file);
static int dev_release(struct inode *inode, struct file *file);
static ssize_t dev_read(struct file *filp, char *buff, size_t len, loff_t *off);
static ssize_t dev_write(struct file *filp, const char *buff, size_t len, loff_t *off);
static long dev_ioctl(struct file *, unsigned int, unsigned long);


/* File operations del driver */
static struct file_operations fops = {
        .owner = THIS_MODULE,
        .write = dev_write,
        .read = dev_read,
        .open =  dev_open,
        .release = dev_release,
        .unlocked_ioctl = dev_ioctl

};

static long dev_ioctl(struct file *filp, unsigned int command, unsigned long param) {

    Session *session = filp->private_data;
    printk("%s: User decided %d command\n", MODNAME, command);
    switch(command){


        case LOW_PRIORITY_IOCTL:

            session->priority = LOW_PRIORITY;
            printk("%s: User decided to set priority to: LOW_PRIORITY\n", MODNAME);
            break;

        case HIGH_PRIORITY_IOCTL:

            session->priority = HIGH_PRIORITY;
            printk("%s: User decided to set priority to: HIGH_PRIORITY\n", MODNAME);
            break;


        case BLOCKING_IOCTL:

            session->blocking = BLOCKING;
            printk("%s: User decided to set blocking to: BLOCKING\n", MODNAME);
            break;


        case NON_BLOCKING_IOCTL:

            session->blocking = NON_BLOCKING;
            printk("%s: User decided to set blocking to: NON_BLOCKING\n", MODNAME);
            break;


        case TIMEOUT_IOCTL:

            session->timeout = param;
            printk("%s: User decided to set timeout to: %d\n", MODNAME,  param);
            break;

        default:

            printk("%s: Illegal command by user %d\n", MODNAME);


    }

    return 0;
}



/* the actual driver */

static int dev_open(struct inode *inode, struct file *file) {

    Minor = get_minor(file);

    Session *session;

    if(Minor >= MINORS){
        return -ENODEV;
    }

    session = kzalloc(sizeof(session), GFP_ATOMIC);
    printk("%s: ALLOCATED new session\n", MODNAME);
    if (session == NULL)
    {
        printk("%s: unable to allocate new session\n", MODNAME);
        return -ENOMEM;
    }
    

    session->priority = HIGH_PRIORITY;
    session->blocking = NON_BLOCKING;
    session->timeout = 0;
    file->private_data = session;



    printk("%s: device file successfully opened for object with minor %d\n", MODNAME, Minor);
    return 0;

}


static int dev_release(struct inode *inode, struct file *file){


    Session *session = file->private_data;
    kfree(session);

    printk("%s: device file closed\n", MODNAME);

    return 0;
}



static ssize_t dev_write(struct file *filp, const char *buff, size_t len, loff_t *off) {


    size_t new_bytes;
    //int minor = get_minor(filp);
    Object_state* object = &objects[Minor];

    Session *session = filp->private_data;
    Flow *flow = &object->flows[session->priority];


    if (len > object->available_bytes){
        printk("%s: non enough available bytes in device driver\n", MODNAME);
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




static ssize_t dev_read(struct file *filp, char *buff, size_t len, loff_t *off){

    Object_state* object = &objects[Minor];
    int bytes_read;
    Session *session = filp->private_data;
    Flow *flow = &object->flows[session->priority];




    if (session->priority == HIGH_PRIORITY){

        if (session->blocking == BLOCKING){
            printk("%s: somebody called a BLOCKING HIGH-PRIORITY read on dev with " \
         "[major,minor] number [%d,%d]\n", MODNAME, Major, Minor);
        }
        else{
            printk("%s: somebody called a NON-BLOCKING HIGH-PRIORITY read on dev with " \
         "[major,minor] number [%d,%d]\n", MODNAME, Major, Minor);
        }

        bytes_read = read(object, session, buff, len, HIGH_PRIORITY, Minor);

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

        bytes_read = read(object, session, buff, len, LOW_PRIORITY, Minor);


    }



    return bytes_read;
}



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
        objects[i].available_bytes = 1000000;
        enabled_device[i]=ENABLED;


    }

    Major = __register_chrdev(0, 0, 256, DEVICE_NAME, &fops);
    //actually allowed minors are directly controlled within this driver

    if (Major < 0) {
        printk("%s: registering device failed\n",MODNAME);
        return Major;
    }

    printk("%s: new device registered, it is assigned major number %d\n",MODNAME, Major);

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


void cleanup_module(void)
{

    int i, j;

    for (i = 0; i < MINORS; i++)
    {
        for (j = 0; j < FLOWS; j++)
        {
            kfree(objects[i].flows[j].obj_head->stream_content);
            kfree(objects[i].flows[j].obj_head);
        }
    }

    unregister_chrdev(Major, DEVICE_NAME);

    printk("%s: new device unregistered, it was assigned major number %d\n", MODNAME, Major);

}


