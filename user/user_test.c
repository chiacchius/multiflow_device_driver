/**
    Programma per il testing utente del multiflow device driver
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>


#define MINORS 128


#define LOW_PRIORITY 0
#define HIGH_PRIORITY 1

#define BLOCKING 0
#define NON_BLOCKING 1


#define MAX_BYTES 1000000

void show_operations();


typedef struct settings{

    int priority;
    int blocking;
    int timeout;

}Settings;








char buff[4096];

Settings settings;

int main(int argc, char *argv[]){

    char* path;
    int ret;
    int major, minor;
    int op;
    char *buff;
    char write_buff[1000000];

    system("clear");

    printf("*----------------------------------------------------------*\n");
    printf("*----------------------------------------------------------*\n");
    printf("*---------**************************************-----------*\n");
    printf("*---------** MULTI-FLOW DEVICE DRIVER TESTING **-----------*\n");
    printf("*---------**************************************-----------*\n");
    printf("*----------------------------------------------------------*\n");
    printf("*----------------------------------------------------------*\n\n\n");

    if (argc!=4){
        printf("Usage: sudo ./user [DEVICE_PATH] *Major number* *Minor number*\n\n");
        exit(-1);
    }

    path = argv[1];
    major = strtol(argv[2], NULL, 10);
    minor = strtol(argv[3], NULL, 10);

    for (int i = 0; i < MINORS; i++)
    {
        sprintf( buff, "mknod %s%d c %d %i 2> /dev/null\n", path, i, major, i);
        system(buff);

    }

    //salvo in buff il nome del device dedicato allo user
    sprintf(buff, "%s%d", path, minor);

    printf("(NOTICE: 128 devices were created. Your is %d in [0;127])\n", minor);


    char *device = (char *)strdup(buff);

    int fd = open(device, O_RDWR);

    if (fd == -1)
    {
        printf("open error on device %s, %s\n", device, strerror(errno));
        return -1;
    }

    //default settings inizializzati
    settings.priority = HIGH_PRIORITY;
    settings.blocking = NON_BLOCKING;
    settings.timeout = 0;

    printf("Default settings:\n");
    printf("\t- Priority: HIGH_PRIORITY\n");
    printf("\t- Blocking: NON_BLOCKING\n");
    printf("\t- Timeout: %d\n", settings.timeout);

    /*char prova[128];
    sprintf(prova, "echo 'Hello' > /dev/multiflow_device%d\n", minor);
    printf("Hai scritto: %s\n", prova);
    system(prova);*/





    while(1){


        show_operations();
        scanf("%d", &op);



        switch(op){

            case 1: //write 

            memset(write_buff, 0, MAX_BYTES);
            printf("What do you want to write?\n");
            scanf("%[^\n]s, write_buff);
            ret = write(fd, write_buff, strlen(write_buff));
            if (ret==-1) printf("Could not write on device: %s\n", strerror(errno));
            else printf("Written %d bytes on device\n", strlen(write_buff));
            break;


            case 2: //read


            case 3: //get_settings


            case 4: break;



        }

      






    }
    

    close(fd);
    return NULL;


    return 0;
}


void show_operations(){
	
	printf("*----------------------------------------------------------*\n");
	printf("*------------------------- OPERATIONS ---------------------*\n");
    printf("*----------------------------------------------------------*\n");
	printf("select the number corresponding to the operation you want to carry out\n");
	printf("1) write on device\n2) read from device\n3) change settings\n4) exit\n");
	printf("*----------------------------------------------------------*\n");
    printf("*----------------------------------------------------------*\n");
    printf("*----------------------------------------------------------*\n");


}
