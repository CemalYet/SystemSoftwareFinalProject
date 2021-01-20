#ifndef MAIN_H_
#define MAIN_H_


#define _GNU_SOURCE 

#include "connmgr.h"
#include "sbuffer.h"
#include "config.h"
#include "datamgr.h"
#include "sensor_db.h"
#include "errmacros.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#define REC_BUF_SIZE 400
#define WAIT_TIME 5      
#define NR_OF_TRIES 3

#define THREAD_RUNNING 1
#define THREAD_ENDED 0

#define CONMGR_THREAD 0
#define DATAMGR_THREAD 1
#define STORAGE_THREAD 2


#define FIFO_NAME 	"logFifo"    
#define FIFO_LOG    "gateway.log"	
#define CLEAR_DB 1

#ifndef TIMEOUT
  #error TIMEOUT not specified!(in seconds)
#endif

//global variables
pthread_t threads[3];
sbuffer_t* sbuffer;
pthread_mutex_t fifolock;
FILE *fpread; 
FILE *fpwrite; 

/*
* This method handles the conmgr
*/
void* start_conmgr_thread(void * port);

/*
* This method handles the datamgr
*/
void* start_datamgr_thread(void * arg);

/*
* This method handles the storagemgr 
and tries to connect DB three times if first try failed.
It finished gateway process when three connection tries failed
*/
void* start_storagemgr_thread(void * arg);

/*
* main checks if the user has given a port and creates the child process
*/
int main( int argc, char *argv[] );

/*
* This method reads log messages from FIFO as a child process
and write them to gateway.log file
*/
void run_child (void);

/*
* This method write log events to FIFO in the parent process
*/
void write_to_FIFO(char *message,FILE *fpw);

/*
* Helps users to start program with valid arguments.  
*/
void print_help(void);



#endif  //MAIN_H_
