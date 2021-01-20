#ifndef __CONMGR_H__
#define __CONMGR_H__
#define _GNU_SOURCE 
#define FIRST_SENSOR_ID 0
#define BEGINING_SIZE 4
#define DECREASE_SIZE 5


#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <assert.h>
#include <inttypes.h>


#include "lib/tcpsock.h"
#include "lib/dplist.h"
#include "config.h"
#include "sbuffer.h"
#include "errmacros.h"
#include "main.h"

#ifndef TIMEOUT
  #error TIMEOUT not specified
#endif


typedef struct pollfd poll_d;

typedef struct{
  int conn_size;	
  poll_d *fd; 
  tcpsock_t* socket;
  time_t last_record;
} server_t;

typedef struct{
  sensor_id_t sensor_id;
  tcpsock_t* socket;
  time_t last_record;
  
} sensor_socket_t;

//global variable
dplist_t *connections_to_server;
server_t *server;

/*
 * This method creates the server object,sets the begining of the connection size  
 * opens TCP connection 
 */
void server_create(int port_number);

/* This method holds the core functionality of the process 
 * it creates the connection list and inserts the coming sensor data into shared buffer 
 * terminates the connection if the timeout is excessed.
 * terminates the process if clients close connection
 * writes log messages into FIFO
 */
void manage_connection(FILE *fp,sbuffer_t** sbuffer,int* thread_result_connmgr,FILE*fpwrite);

/* 
 * Opens and writes sensor data to the sensor_data_recv
 * Calls server_create function and manage_connection function to start to process
 */
void connmgr_listen(int port_number,sbuffer_t** sbuffer,int* thread_result_connmgr,FILE*fpwrite);

/*
 * When the connection is removed from list it arranges the poll array indexes to  conn. list
 */
void shift_array(int index,int size,poll_d* poll_fd);

/*
 * This method should be called to clean up the connmgr, and to free all used memory.
 * After this no new connections will be accepted
 */
void connmgr_free();







#endif  //__CONMGR_H__