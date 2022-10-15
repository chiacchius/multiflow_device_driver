/**
    Programma per il testing utente del multiflow driver driver
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

#define MAX_DEVICE_BYTES 1000000
#define MAX_BYTES 1024

#define ENABLED_PATH "/sys/module/multiflow_driver/parameters/enabled_device"
#define HP_BYTES_PATH "/sys/module/multiflow_driver/parameters/hp_bytes"
#define LP_BYTES_PATH "/sys/module/multiflow_driver/parameters/lp_bytes"
#define HP_THREADS_PATH "/sys/module/multiflow_driver/parameters/hp_threads"
#define LP_THREADS_PATH "/sys/module/multiflow_driver/parameters/lp_threads"

void show_operations();
void new_settings();
void get_status();
int find_value(char* path, int minor);
void change_enabling();

typedef struct settings{

    int priority;
    int blocking;
    int timeout;

}Settings;



char buff[MAX_BYTES];
char rw_buff[MAX_BYTES];
int bytes_num;
int fd;
int Major, Minor;
char* path;


Settings settings;

int main(int argc, char *argv[]){

    
    int ret;
    
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
    Major = strtol(argv[2], NULL, 10);
    Minor = strtol(argv[3], NULL, 10);

    for (int i = 0; i < MINORS; i++)
    {
        sprintf(buff, "mknod %s%d c %d %i 2> /dev/null\n", path, i, Major, i);
        system(buff);

    }

    //salvo in buff il nome del driver dedicato allo user
    sprintf(buff, "%s%d", path, Minor);

    printf("(NOTICE: 128 devices were created. Your is %d in [0;127])\n", Minor);


    char *device = (char *)strdup(buff);

    fd = open(device, O_RDWR);

    if (fd == -1)
    {
        printf("open error on driver %s, %s\n", device, strerror(errno));
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

  


    while(operation!=6){

	
        show_operations();
        memset(op, 0, 10);
        fgets(op, sizeof(op), stdin);
	    operation = atoi(op);
	
	
        switch(operation){
        

            case 1: //write 
           

                memset(rw_buff, 0, MAX_BYTES);
                printf("What do you want to write?: ");
                fgets(rw_buff, sizeof(rw_buff), stdin);
                rw_buff[strcspn(rw_buff, "\n")] = 0;
                ret = write(fd, rw_buff, min(MAX_BYTES, strlen(rw_buff)));
                if (ret==-1) printf("Could not write on driver");
                else printf("Written %ld bytes on driver: %s\n", min(MAX_BYTES, strlen(rw_buff)), rw_buff);
            
            
            
            break;


            case 2: //read


                memset(buff, 0, MAX_BYTES);
                memset(rw_buff, 0, MAX_BYTES);
                printf("\nHow many bytes do you want to read?: ");
                fgets(buff, sizeof(buff), stdin);
                bytes_num = atoi(buff);
                ret = read(fd, rw_buff, min(MAX_BYTES, bytes_num));
                if (ret==-1) printf("Could not read from driver\n");
                else printf("Read %ld bytes from driver: \n%s\n", min(MAX_BYTES, strlen(rw_buff)), rw_buff);


            break;


            case 3: //cambiamento dei valori della sessione

                new_settings();

                break;
            
            case 4: //stato attuale del device
            
            	get_status(Major, Minor);
            	
            	break;

            case 5: //abilitare o disabilitare un device file a scelta
            
                change_enabling();
                
                break;

            case 6:
            
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


    system("clear");
    printf("*----------------------------------------------------------*\n");
    printf("YOU CAN CHOOSE YOUR SESSION SETTINGS:\n");
    printf("1) Set priority\n");
    printf("2) Set blocking and timeout\n");
    printf("*----------------------------------------------------------*\n");


    while(1){

        //scanf("%s", op);
        memset(op, 0, 10);
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
                
                operation +=2;
                memset(dec, 0, 10);
                settings.priority = decision;
                
          
                
               

                ret = ioctl(fd, 3, decision-1);
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
                operation +=2;
                memset(dec, 0, 10);


                settings.blocking = decision;
                
                if (decision==1){

                    printf("Decide how long you can wait for blocking operation completion: ");
            	    fgets(dec, sizeof(dec), stdin);
          
                    dec[strcspn(dec, "\n")] = 0;
                    decision = atoi(dec);
                    memset(dec, 0, 10);
                    settings.timeout = decision;
                    ret = ioctl(fd, 4, decision);
                    if (ret == -1) goto exit;

                }
                else{
                    printf("Your decision is non-blocking operation so timeout is set to 0 ");
            	    settings.timeout = 0;
                    ret = ioctl(fd, operation, 0);
                    if (ret == -1) goto exit;
                }
                
               
                goto stop;
                
                
                
              

            
            default:

                printf("Illegal command: retry\n");


        }
    


    }



    exit: 
        printf("Error on ioctl() (%s)\n", strerror(errno));
        close(fd);
        exit(-1);
    stop:
	    return;




}



void get_status(){

	printf("Actually in driver with Major = %d and Minor = %d we have:\n", Major, Minor);
	
	char Device[30];
    sprintf(Device, "Device[%d][%d]", Major, Minor);
	
	printf("*-----------------------------------------------------------------*\n");
	printf("* %s: Timeout = %d                                                \n" , Device, settings.timeout);
	printf("* %s: Available bytes = %d                                        \n", Device,  MAX_DEVICE_BYTES- find_value(HP_BYTES_PATH, Minor) - find_value(LP_BYTES_PATH, Minor));
	printf("* %s: Number of bytes in high priority flow = %d                  \n" , Device, find_value(HP_BYTES_PATH, Minor));
	printf("* %s: Number of bytes in low priority flow = %d                   \n" , Device, find_value(LP_BYTES_PATH, Minor));
	
	printf("* %s: Number of waiting threads in high priority flow = %d        \n" , Device, find_value(HP_THREADS_PATH, Minor));
	printf("* %s: Number of waiting threads in low priority flow = %d         \n" , Device, find_value(LP_THREADS_PATH, Minor));
	
	printf("*-----------------------------------------------------------------*\n");
	
	
	
	
	
	
	
	
	

}


int find_value(char *path, int minor){
	
	int value,it;
	
	
	FILE* file_stream = fopen(path, "r");
	char string[2048];
	fgets(string, 2048, file_stream);
	
    char * token = strtok(string, ",");

    if (minor==0){
        return atoi(token);
    }

    it=0;
    while( it!=minor ) {
        token = strtok(NULL, ",");
        //printf( "%s\n", token ); //printing each token
        it++;

    }

    value = atoi(token);
    //printf("%d\n", value);
	
	fclose(file_stream);
	

	return value;
}

void change_enabling(){
    
    char dec[10];
    char command[64];
    printf("Decide the minor number you want to change enabling: ");
    int minor, enable;
    //scanf("%d", &minor);
    fgets(dec, sizeof(dec), stdin);
          
    dec[strcspn(dec, "\n")] = 0;
    minor = atoi(dec);
    memset(dec, 0, 10);
    printf("%d\n", minor); 
    enable = find_value(ENABLED_PATH, minor);
    printf("%d\n", enable); 
    
    if (enable == 0){
        printf("Actually the device file is DISABLED, do you want to change the enabling? (y/n): ");

    }

    else{
        printf("Actually the device file is ENABLED, do you want to change the enabling? (y/n): ");
    }


    fgets(dec, sizeof(dec), stdin);
    dec[strcspn(dec, "\n")] = 0;
                
                
                

    if (strcmp(dec, "n")==0){

         memset(dec, 0, 10);
         return;
                    
    }
    memset(dec, 0, 10);
    char new_enable_status;
    
    if (enable == 0){
        
        //sprintf(command, "echo 1 > %s", path);
        //system(command);
        new_enable_status = '1';

    }
    else {
        //sprintf(command, "echo 0 > %s", path);
        //system(command);
        new_enable_status = '0';
    }

    FILE* file_stream = fopen(ENABLED_PATH, "w+");
	char *string=NULL;
    size_t string_len = 0;
	getline(&string, &string_len, file_stream);
   

    string[2*minor] = new_enable_status;

    fputs(string, file_stream);
    fclose(file_stream);
    


    memset(command, 0, 64);
         

    



}

void show_operations(){
	
	printf("*----------------------------------------------------------*\n");
	printf("*------------------------- OPERATIONS ---------------------*\n");
    printf("*----------------------------------------------------------*\n");
	printf("select the number corresponding to the operation you want to carry out\n");
	printf("1) write on driver\n2) read from driver\n3) change settings\n4) get driver status\n5) change enabling\n6) exit\n");
	printf("*----------------------------------------------------------*\n");
    printf("*----------------------------------------------------------*\n");
    printf("*----------------------------------------------------------*\n");


}
