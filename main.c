#include "main.h"

int thread_result_connmgr= THREAD_RUNNING;
/*
 inspired from pthread Tutorial Peter C. Chapin p.4 section 2.1 
 creating threads 
*/
void* start_conmgr_thread(void * port){
    
    /*port string to int*/ 
    int server_port = atoi(port); 

    connmgr_listen(server_port,&sbuffer,&thread_result_connmgr,fpwrite);
    
    /*set to flag end when there is not any active TCP conn.*/
    thread_result_connmgr = THREAD_ENDED;
    
    #ifdef DEBUG
    printf("conmgr is  ended \n");
    #endif
    
    return NULL;
}

void* start_datamgr_thread(void * arg){
    
    int file_result;
    
    /*opens room_sensor.map file*/
    FILE * fp= fopen("room_sensor.map","r");  
    FILE_OPEN_ERROR(fp);

    datamgr_parse_sensor_data(fp, &sbuffer,&thread_result_connmgr,fpwrite);

    file_result=fclose(fp);
    FILE_CLOSE_ERROR(file_result);

    //datamgr_free();

    #ifdef DEBUG
    printf("datamgr is ended\n");
    #endif

    return NULL;
}

void* start_storagemgr_thread(void * arg){    

    DBCONN *conn;
    conn=NULL;
    int nr_of_tries=0;

    /*try to connect DB three times*/
    do
    {   
        conn = init_connection(CLEAR_DB,fpwrite); 
        
        /*connection successfull*/
        if (conn!=NULL)
        {   
            get_sbuffer_data(conn, &sbuffer,&thread_result_connmgr); 
            disconnect(conn,fpwrite); 
            return NULL;
        }
        /*conn. failed then wait*/ 
        if (nr_of_tries<2)
        {   
            
           sleep(WAIT_TIME);
           
        }
        nr_of_tries++;
    } while (nr_of_tries < NR_OF_TRIES);

    /*if connection fails after three attempt connmgr is finished */
    thread_result_connmgr = THREAD_ENDED;

    /*if connection fails after three attempt datamgr is finished*/
    finish_datamgr(&sbuffer,&thread_result_connmgr);

    #ifdef DEBUG
    printf("storage manager ended\n");
    #endif

    return NULL;
}



void run_child ()
{   
    /*read log messages from FIFO */ 
    
    /*
    code was inspired from lecture 
    process_code-section #fifo_reader.c
    */
    char *str_result;
    char recv_buf[REC_BUF_SIZE];
    int result;
    int sequence_number=0;

    FILE *flog;

    /* Create the FIFO if it does not exist */ 
    result = mkfifo(FIFO_NAME, 0666);
    CHECK_MKFIFO(result); 

    fpread = fopen(FIFO_NAME, "r"); 
    FILE_OPEN_ERROR(fpread);

    #ifdef DEBUG
    printf("syncing with writer ok\n");
    #endif

    flog = fopen(FIFO_LOG, "w"); 
    FILE_OPEN_ERROR(fpread);
   
    #ifdef DEBUG
    printf("syncing with writer to gateway.log file ok\n");
    #endif
  
   
   do 
  {  /*Reads log messages from FIFO*/
    str_result = fgets(recv_buf, REC_BUF_SIZE, fpread);
 
    if ( str_result != NULL )
    {  
        #ifdef DEBUG
        //printf("Message received: %s", recv_buf);
        #endif

        sequence_number++;
        /*Writes log messages to gateway.log*/
        fprintf(flog,"%d %ld %s",sequence_number, time(NULL), recv_buf);
    }
  } while ( str_result != NULL ); 


    result = fclose( fpread );
    FILE_CLOSE_ERROR(result);

    result = fclose( flog );
    FILE_CLOSE_ERROR(result);
    exit(EXIT_SUCCESS); 
}

void write_to_FIFO(char *message,FILE *fpw){
    
    /* inspired from lecture process_code 
    section #fifo_writer.c 
    */
   
    pthread_mutex_lock(&fifolock); 
     /*thread safe region*/
    if(fputs( message, fpwrite) == EOF)
    {
        fprintf( stderr, "Error writing data to fifo\n");
        exit( EXIT_FAILURE );
    } 
    FFLUSH_ERROR(fflush(fpwrite));
    #ifdef DEBUG
    //printf("Message send: %s", message); 
    #endif
    
    free( message );  
    /*thread safe region*/ 
    pthread_mutex_unlock(&fifolock); 
}



int main( int argc, char *argv[] ){
    /*
    inspired from lecture process_code section #wait.c 
    */
    pid_t my_pid,child_pid;
    int child_exit_status;
    int result;

    if (argc != 2) 
    {
        print_help();
        exit(EXIT_SUCCESS);
    }
    /*Create child process*/
    my_pid = getpid();
    child_pid = fork();
    SYSCALL_ERROR(child_pid);
   
    if(child_pid == 0)
    {  
        run_child();

    }else{  
        
        /*
        parent’s code section
        */
        printf("Gateway is started (pid = %d) is started ...\n", my_pid);
        
        /*Initialize shared buffer,barriers,rw_locks */
        sbuffer_init(&sbuffer);
       
        /* Create the FIFO if it does not exist */ 
        result = mkfifo(FIFO_NAME, 0666);
        CHECK_MKFIFO(result); 

        fpwrite = fopen(FIFO_NAME, "w");
        FILE_OPEN_ERROR(fpwrite);

        #ifdef DEBUG 
        printf("syncing with reader ok\n");
        #endif

        /*mutex for FIFO write*/

        if (pthread_mutex_init(&fifolock, NULL) != 0)
        {
            printf("\n mutex init has failed\n");
        }
      
        /*
        inspired from pthread Tutorial Peter C. Chapin p.4 section 2.1 
        creating threads 
        */
        
        if (pthread_create(&(threads[CONMGR_THREAD]), NULL, &start_conmgr_thread,(void*) argv[1])!=0)
        {
            printf("Connmgr  thread can't be created\n");
            exit(EXIT_FAILURE);
        }

        if (pthread_create(&(threads[DATAMGR_THREAD]), NULL, &start_datamgr_thread, NULL)!=0)
        {
            printf("Datamgr thread can't be created\n");
            exit(EXIT_FAILURE);
        }

        if (pthread_create(&(threads[STORAGE_THREAD]), NULL, &start_storagemgr_thread, NULL)!=0)
        {
            printf("Storagemgr thread can't be created\n");
            exit(EXIT_FAILURE);
        }

        /* 
        program wait for each thread to terminate and collect the result 
        it produced by calling pthread join()
        */ 
        if (pthread_join(threads[CONMGR_THREAD], NULL) !=0)
        {
            printf("Can't join thread Connmgr thread \n");
        }

        if (pthread_join(threads[DATAMGR_THREAD], NULL) !=0)
        {
             printf("Can't join thread Datamgr thread\n");
        }
         
        if (pthread_join(threads[STORAGE_THREAD], NULL) !=0)
        {
            printf("Can't join thread Storagemgr thread\n");
        }
     
        /*Close the FIFO after all threads are joined*/
        result = fclose( fpwrite );
        FILE_CLOSE_ERROR(result);
     
        /*free shared buffer*/
        sbuffer_free(&sbuffer);
        SYSCALL_ERROR( wait(&child_exit_status) );


    // waiting on  child    
    if ( WIFEXITED(child_exit_status) )
    {
        printf("Child %d terminated with exit status %d\n", child_pid, WEXITSTATUS(child_exit_status));
    }
    else
    {
        printf("Child %d terminated abnormally\n", child_pid);
    }
    
    //terminates main thread
    pthread_exit(NULL);

    }
    exit(EXIT_SUCCESS);
}



void print_help(void)
{
        printf("Use this program with 1 command line option: \n");
        printf("\t%-15s : TCP server port number\n", "\'server port\'");
}
