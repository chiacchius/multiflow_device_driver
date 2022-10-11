/**
    Programma per il testing utente del multiflow device driver
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>


#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

#define MINORS 128


#define LOW_PRIORITY 0
#define HIGH_PRIORITY 1

#define BLOCKING 0
#define NON_BLOCKING 1


#define MAX_BYTES 1024

void show_operations();
void new_settings();


typedef struct settings{

    int priority;
    int blocking;
    int timeout;

}Settings;








char buff[MAX_BYTES];
char rw_buff[MAX_BYTES];
int bytes_num;
int fd;



Settings settings;

int main(int argc, char *argv[]){

    char* path;
    int ret;
    int major, minor;
    char op[10];
    int operation;
    

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
        sprintf(buff, "mknod %s%d c %d %i 2> /dev/null\n", path, i, major, i);
        system(buff);

    }

    //salvo in buff il nome del device dedicato allo user
    sprintf(buff, "%s%d", path, minor);

    printf("(NOTICE: 128 devices were created. Your is %d in [0;127])\n", minor);


    char *device = (char *)strdup(buff);

    fd = open(device, O_RDWR);

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

  


    while(operation!=4){


        show_operations();
        memset(op, 0, 10);
        fgets(op, sizeof(op), stdin);
	    operation = atoi(op);
	
	
        switch(operation){
        

            case 1: //write 
            

            
	        

            memset(rw_buff, 0, MAX_BYTES);
            printf("What do you want to write?: ");
            fgets(rw_buff, sizeof(rw_buff), stdin);
            ret = write(fd, rw_buff, min(MAX_BYTES, strlen(rw_buff)));
            if (ret==-1) printf("Could not write on device");
            else printf("Written %ld bytes on device: %s\n", min(MAX_BYTES, strlen(rw_buff)), rw_buff);
            
            break;


            case 2: //read


            memset(buff, 0, MAX_BYTES);
            memset(rw_buff, 0, MAX_BYTES);
            printf("\nHow many bytes do you want to read?: ");
            fgets(buff, sizeof(buff), stdin);
            bytes_num = atoi(buff);
            ret = read(fd, rw_buff, min(MAX_BYTES, bytes_num));
            if (ret==-1) printf("Could not read from device\n");
            else printf("Read %ld bytes from device: %s\n", min(MAX_BYTES, strlen(rw_buff)), rw_buff);


            break;


            case 3: 

            new_settings();

            break;

            case 4: 
            
            break;


            default:

            printf("Illegal command: retry\n");




        }

      






    }
    

    close(fd);

    return 0;
}


void new_settings(){

    char op[10];
    int operation=0;
    char dec[10];
    int decision;
    int ret;
    int timeout;

    system("clear");
    printf("*----------------------------------------------------------*\n");
    printf("YOU CAN CHOOSE YOUR SESSION SETTINGS:\n");
    printf("1) Set priority\n");
    printf("2) Set blocking\n");
    printf("3) Set timeout\n");
    printf("*----------------------------------------------------------*\n");


    while(operation!=4){


        //scanf("%s", op);
        memset(dec, 0, 10);
	fgets(op, sizeof(op), stdin);
        operation = atoi(op);

        switch(operation){


            case 1:

                
                printf("Decide what type of priority you want:\n");
                printf("1) LOW_PRIORITY\n");
                printf("2) HIGH_PRIORITY\n");
                //scanf("%s", dec);
                fgets(dec, sizeof(dec), stdin);
                dec[strcspn(dec, "\n")] = 0;
                decision = atoi(dec);
                memset(dec, 0, 10);
                settings.priority = decision;
                
                printf("your decision: +%d+\n", decision);

                ret = ioctl(fd, decision, timeout);
                if (ret == -1) goto exit;
                
                
                printf("Do you want to continue changing settings? (y/n): ");
                //scanf("%s", dec);
                fgets(dec, sizeof(dec), stdin);
                dec[strcspn(dec, "\n")] = 0;
                printf("%s+\n", dec);

                if (strcmp(dec, "n")==0)
                {
                    memset(dec, 0, 10);
                    
                    goto stop;
                }
                memset(dec, 0, 10);

            case 2:


                
                printf("Decide what type of blocking you want:\n");
                printf("1) BLOCKING\n");
                printf("2) NON_BLOCKING\n");
                //scanf("%s", dec);
                fgets(dec, sizeof(dec), stdin);
                dec[strcspn(dec, "\n")] = 0;
                decision = atoi(dec);
                memset(dec, 0, 10);

                settings.blocking = decision;

                decision += 2;
                
                printf("your decision: %d\n", decision);


                ret = ioctl(fd, decision, timeout);
                if (ret == -1) goto exit;
                
                
                printf("Do you want to continue changing settings? (y/n): ");
                //scanf("%s", dec);
                fgets(dec, sizeof(dec), stdin);
                dec[strcspn(dec, "\n")] = 0;
                

                if (strcmp(dec, "n"))
                {
                    memset(dec, 0, 10);
                    goto stop;
                }
                memset(dec, 0, 10);
                

            case 3:

                break;

            default:

                printf("Illegal command: retry\n");


        }
    
exit: 
        printf("Error on ioctl() (%s)\n", strerror(errno));
        close(fd);
        exit(-1);
stop:
	return;

    }








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
