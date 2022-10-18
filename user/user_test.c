/**
 * 
 * USER PROGRAM TO TEST THE MULTIFLOW DEVICE DRIVER
 * 
 * Programma utente per l'utilizzo del driver implementato.
 * Tutte le operazioni richieste possono essere effettuate 
 * utilizzando questa semplice e intuitiva interfaccia a riga
 * di comando.
 * Eseguendo il programma di creano 128 nodi corrispondenti ai 
 * minor numbers gestibili dal device driver per poi aprire una 
 * sessione di I/O con uno di questi specificato durante il lancio
 * dell'applicativo.
 * Le operazioni eseguibili sono:
 * 1) Scrivere sul device
 * 2) Leggere dal device.
 * 3) Cambiare tipo di sessione
 * 4) Leggere i parametri del device file con cui si è aperta una sessione
 * 5) Cambiare l'abiitazione di un device file a scelta
 * 6) Chiudere la sessione e terminare il programma
 * 
 * Author: Matteo Chiacchia (0300177) 
 * 
 * 
 * */


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

#define MAX_DEVICE_BYTES 1000000 //Numero max di bytes gestibili dal device file
#define MAX_BLOCK_BYTES 1024 //Numero max di bytes che si possono scrivere su un unico blocco

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

typedef struct session_settings{

    int priority;
    int blocking;
    int timeout;

}Settings;



char buff[MAX_BLOCK_BYTES];
char rw_buff[MAX_BLOCK_BYTES];
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
        printf("Usage: sudo ./user [DEVICE_PATH] [Major_number] [Minor number]\n\n");
        exit(-1);
    }

    path = argv[1];
    Major = strtol(argv[2], NULL, 10);
    Minor = strtol(argv[3], NULL, 10);

    // creazione dei 128 nodi che il device driver è in grado di gestire
    for (int i = 0; i < MINORS; i++)
    {
        sprintf(buff, "mknod %s%d c %d %i\n", path, i, Major, i);
       
        system(buff);

    }

    //salvo in buff il nome del driver dedicato allo user
    sprintf(buff, "%s%d", path, Minor);

    printf("\n(NOTICE: 128 devices were created. Your is %d in [0;127])\n", Minor);


    char *device = (char *)strdup(buff);

    fd = open(device, O_RDWR); //apertura della sessione di I/O

    if (fd == -1)
    {
        printf("open error on driver %s, %s\n", device, strerror(errno));
        return -1;
    }

    //settings della sessione di default
    settings.priority = HIGH_PRIORITY;
    settings.blocking = NON_BLOCKING;
    settings.timeout = 0;

    printf("Default settings:\n");
    printf("\t- Priority: HIGH_PRIORITY\n");
    printf("\t- Blocking: NON_BLOCKING\n");
    printf("\t- Timeout: %d\n", settings.timeout);

  

    // fine inizializzazione ed effettivo utilizzo del device
    while(operation!=6){

	
        show_operations();
        memset(op, 0, 10);
        fgets(op, sizeof(op), stdin);
	    operation = atoi(op);
	
	
        switch(operation){
        

            case 1: //operazione di write 
           

                memset(rw_buff, 0, MAX_BLOCK_BYTES);
                printf("What do you want to write?: ");
                fgets(rw_buff, sizeof(rw_buff), stdin);
                rw_buff[strcspn(rw_buff, "\n")] = 0;
                ret = write(fd, rw_buff, min(MAX_BLOCK_BYTES, strlen(rw_buff)));
                if (ret==-1) printf("Could not write on driver");
                else printf("Written %ld bytes on driver: %s\n", min(MAX_BLOCK_BYTES, strlen(rw_buff)), rw_buff);
            
            
            
            break;


            case 2: //operazione di read


                memset(buff, 0, MAX_BLOCK_BYTES);
                memset(rw_buff, 0, MAX_BLOCK_BYTES);
                printf("\nHow many bytes do you want to read?: ");
                fgets(buff, sizeof(buff), stdin);
                bytes_num = atoi(buff);
                ret = read(fd, rw_buff, min(MAX_BLOCK_BYTES, bytes_num));
                if (ret==-1) printf("Could not read from driver\n");
                else printf("Read %ld bytes from driver: \n%s\n", min(MAX_BLOCK_BYTES, strlen(rw_buff)), rw_buff);


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

            case 6: //terminazione del programma
            
            	break; 
            
            
            


            default: 

            printf("Illegal command: retry\n");




        }

      

	




    }
    

    close(fd); //chiusura della sessione di I/O e terminazione del programma

    return 0;
}

/**
 * 
 * Funzione che chiama la ioctl per cambiare il tipo di sessione.
 * Si può scegliere il livello di priorità e il tipo di operazione: bloccante o non bloccante.
 * Nel caso si scelga un'operazione non bloccante non serve immettere un timeout, altrimenti
 * viene chiesto all'utente di settare anche questo valore
 * 
 * */
void new_settings(){

    char op[10];
    int operation=0;
    char dec[10];
    int decision;
    int ret;


    
    printf("\n\n*----------------------------------------------------------*\n");
    printf("YOU CAN CHOOSE YOUR SESSION SETTINGS:\n");
    printf("1) Set priority\n");
    printf("2) Set blocking and timeout (ms)\n");
    printf("*----------------------------------------------------------*\n\n");


    while(1){


        memset(op, 0, 10);
        memset(dec, 0, 10);
	    fgets(op, sizeof(op), stdin);
        operation = atoi(op);

        switch(operation){


            case 1:

                
                printf("Decide what type of priority you want:\n");
                printf("1) LOW_PRIORITY\n");
                printf("2) HIGH_PRIORITY\n");
  
                fgets(dec, sizeof(dec), stdin);
                dec[strcspn(dec, "\n")] = 0;
                decision = atoi(dec);
                
                operation +=2;
                memset(dec, 0, 10);
                settings.priority = decision-1;
                
          
                
               

                ret = ioctl(fd, 3, decision-1);
                printf("\n%d\n", decision-1);
                if (ret == -1) goto exit;
                
                
                printf("\nDo you want to continue changing settings? (y/n): ");

                fgets(dec, sizeof(dec), stdin);
                dec[strcspn(dec, "\n")] = 0;


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

                fgets(dec, sizeof(dec), stdin);
                dec[strcspn(dec, "\n")] = 0;
                decision = atoi(dec);
                operation +=2;
                memset(dec, 0, 10);


                
                
                if (decision==1){

                    printf("\nDecide how long you can wait for blocking operation completion: ");
            	    fgets(dec, sizeof(dec), stdin);
                    dec[strcspn(dec, "\n")] = 0;
                    decision = atoi(dec);
                    memset(dec, 0, 10);
                    settings.timeout = decision;
                    settings.blocking == BLOCKING;
                    ret = ioctl(fd, 4, decision);
                    if (ret == -1) goto exit;

                }
                else{
                    printf("\nYour decision is non-blocking operation so timeout is set to 0 \n");
                    settings.blocking == NON_BLOCKING;
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


/**
 * 
 * Funzione utilizzata per stampare a schermo i parametri del device
 * insieme ad altre informazioni come il tipo di sessione.
 * 
 * */
void get_status(){

    printf("\n*-------------------------------------------------------------------------------*\n");
    printf("*--------------------- MULTI-FLOW DEVICE DRIVER STATUS --------------------------*\n");
    printf("*--------------------------------------------------------------------------------*\n");   

	printf("\nActually in driver with MAJOR = %d and MINOR = %d we have:\n", Major, Minor);
	
	char Device[30];
    sprintf(Device, "Device[%d][%d]", Major, Minor);
	
	if (settings.priority==LOW_PRIORITY) printf("* %s: Priority = LOW_PRIORITY\n" , Device);
    else printf("* %s: Priority = HIGH_PRIORITY\n" , Device);

    if (settings.blocking == BLOCKING) printf("* %s: Blocking = BLOCKING\n" , Device);
    else printf("* %s: Blocking = NON_BLOCKING\n" , Device);

	printf("* %s: Timeout = %d\n" , Device, settings.timeout);
	printf("* %s: Available bytes = %d\n", Device,  MAX_DEVICE_BYTES - find_value(HP_BYTES_PATH, Minor) - find_value(LP_BYTES_PATH, Minor));
	printf("* %s: Number of bytes in high priority flow = %d\n" , Device, find_value(HP_BYTES_PATH, Minor));
	printf("* %s: Number of bytes in low priority flow = %d \n" , Device, find_value(LP_BYTES_PATH, Minor));
	
	printf("* %s: Number of waiting threads in high priority flow = %d        \n" , Device, find_value(HP_THREADS_PATH, Minor));
	printf("* %s: Number of waiting threads in low priority flow = %d         \n" , Device, find_value(LP_THREADS_PATH, Minor));
	
	printf("*-------------------------------------------------------------------------------*\n");
    printf("*-------------------------------------------------------------------------------*\n");
    printf("*-------------------------------------------------------------------------------*\n");
	
	
	
	
	
	
	
	
	

}



/**
 * 
 * Funzione utilizzata per trovare valore di un determinato parametro
 * relativo al device file con uno specifico minor number
 * 
 * 
 * @path: path del parametro
 * @minor: minor number del device file
 * 
 * 
 * @Returns: int rappresentante il valore del parametro cercato
 * 
 * */
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
       
        it++;

    }

    value = atoi(token);

	
	fclose(file_stream);
	

	return value;
}



/**
 * 
 * Funzione utilizzata per cambiare la disponibilità di un 
 * device file con un minor number richiesto a run-time.
 * Viene stampata l'attuale disponibilità del device file 
 * e si chiede all'utente se la si vuole cambiare.
 * 
 * */
void change_enabling(){
    
    char dec[10];
    char command[64];
    printf("Decide the minor number you want to change enabling: ");
    int minor, enable;

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
        
        
        new_enable_status = '1';

    }
    else {
        
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
