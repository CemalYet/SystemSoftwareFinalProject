
#include "connmgr.h"

/***************************************************
 inspired from lectures process_code section #poll.c
****************************************************/

/*Callback functions for connection_list */
void* callback_copy(void * socket_node){
	sensor_socket_t* copy_socket = malloc(sizeof(sensor_socket_t));
	MALLOC_ERR_HANDLER(copy_socket==NULL,MALLOC_MEMORY_ERROR);
	copy_socket->last_record = ((sensor_socket_t*)socket_node)->last_record;
	copy_socket->sensor_id= ((sensor_socket_t*)socket_node)->sensor_id;
	copy_socket->socket= ((sensor_socket_t*)socket_node)->socket;
	return (void*)copy_socket;
}

void callback_free(void ** socket_node){
	free(*socket_node);
}

int callback_compare(void *node_x,void *node_y){
	return 0;
}


void server_create(int port_number){
	
	/*intialize  server data and poll data*/
	server = (server_t*)malloc(sizeof(server_t));
	MALLOC_ERR_HANDLER(server==NULL,MALLOC_MEMORY_ERROR);

	server->conn_size = BEGINING_SIZE;

	//initialize file desc array in the server
	server->fd = (poll_d*)malloc(sizeof(poll_d)*server->conn_size);
	MALLOC_ERR_HANDLER(server->fd==NULL,MALLOC_MEMORY_ERROR);

	printf("Server is started\n");

	//New socket in passive listening mode
	if (tcp_passive_open(&server->socket, port_number) != TCP_NO_ERROR) {
		
		printf("Problem is occured when try to connect to server\n");
		exit(EXIT_FAILURE);
	}
	//Get the server socket descriptor and put into to server fd field
	if(tcp_get_sd(server->socket,&(server->fd[0].fd))!= TCP_NO_ERROR){

		printf("Unable to obtain socket descriptor\n");
		exit(EXIT_FAILURE);
	}
	/*The server listens to POLLIN if there is data available */
	server->fd[0].events= POLLIN | POLLRDHUP ;

	/*Save the last event time in the server*/
	server->last_record=time(NULL);
	
}

void manage_connection(FILE *fp,sbuffer_t** sbuffer,int* thread_result_connmgr,FILE*fpwrite){
	
	bool exit_flag=false;
	char *log_wbuff;
	
	/* Create connection list */
	connections_to_server = dpl_create(callback_copy,callback_free,callback_compare);
	

	while(!exit_flag && *thread_result_connmgr !=0){
		
		//fd array size
		int conn_size = server->conn_size;

		//sensor connections
		int list_size=dpl_size(connections_to_server);

		//check if there is any revents in each socket conns. then fill the poll array with POLLIN or POLLRDHUP etc. revents.** fd[0] is server **
		poll(server->fd,list_size+1,0);

		//if the connection size excess the begining size increase the poll array size
		if (list_size == conn_size-1)
		{
		
			server->fd = realloc(server->fd, (conn_size*2) * sizeof(poll_d*));
			server->conn_size=conn_size*2;
		
			if (server->fd == NULL)
			{
    			printf("fd_array could not be resized\n");
			}

		//if the connection size is diminished decrease the increased array size to save more place in the  memory
		}else if(list_size==conn_size-DECREASE_SIZE){
			
			server->fd = realloc(server->fd, (list_size+1) * sizeof(poll_d*));
			server->conn_size=list_size+1;
		
			if (server->fd == NULL)
			{
    			printf("fd_array could not be resized\n");
			}
		}

		//check if there is new sensor conn. request to server(adding new sensor node)
		if (server->fd[0].revents & POLLIN)
		{
			//initialize new sensor socket struct and wait the connec.
			sensor_socket_t new_sensor;
			tcp_wait_for_connection(server->socket, &new_sensor.socket);

			//first element is server
			if (tcp_get_sd(new_sensor.socket,&(server->fd[list_size+1].fd)) != TCP_NO_ERROR)
			{
				printf("connection couldn't provide\n");
			}
			//set the new socket connection to listen POLLIN or POLLHUP(if conn. closed)
			server->fd[list_size+1].events= POLLIN | POLLRDHUP;

			new_sensor.last_record = time(NULL);

			//set new connection sensor id 0 to be able to send new connection message to FIFO 
			new_sensor.sensor_id = FIRST_SENSOR_ID;

			//insert to coming conn to list
			connections_to_server = dpl_insert_at_index(connections_to_server,(void*)(&new_sensor),dpl_size(connections_to_server),true); 
			
		}
		//close conn. and free conn. if there is not incoming conn or timeout excessed
		if (list_size==0 && time(NULL)>(server->last_record+TIMEOUT) )
			{
				printf("There is no active client \n");
				tcp_close(&(server->socket));
				connmgr_free();
				exit_flag=true;
				break;
			}
		//loop the connection list and write coming sensor data to shared buffer
		for (int i = 0; i < list_size; ++i)
		{	
			
			sensor_socket_t *sensor=((sensor_socket_t*)dpl_get_element_at_index(connections_to_server,i));
			
			if (server->fd[i+1].revents&POLLIN)
			{
				
				sensor=((sensor_socket_t*)dpl_get_element_at_index(connections_to_server,i));

				sensor_data_t data;
    			int bytes, result;
				
				// read sensor ID
            	bytes = sizeof(data.id);
            	result = tcp_receive(sensor->socket, (void *) &data.id, &bytes);
            			
            	// read temperature
            	bytes = sizeof(data.value);
            	result = tcp_receive(sensor->socket, (void *) &data.value, &bytes);
            	// read timestamp
            	bytes = sizeof(data.ts);
            	result = tcp_receive(sensor->socket, (void *) &data.ts, &bytes);

            	
            	if ((result == TCP_NO_ERROR) && bytes) {
            		
            		sensor->last_record=data.ts;
            		server->last_record=data.ts;

                    //write in sensor_data_recv
                    fprintf(fp, "%d %f %ld\n", data.id, data.value, data.ts);

                    //insert sensor data to shared buffer
                    sbuffer_insert(*sbuffer,&data);
                    #ifdef DEBUG
                	printf("sensor id = %" PRIu16 " - temperature = %g - timestamp = %ld\n", data.id, data.value,
                    (long int) data.ts);
                	#endif
            	}
            	if (sensor->sensor_id==FIRST_SENSOR_ID)
                {	
       				//write new conn. to FIFO
                   	sensor->sensor_id=data.id;
        			asprintf( &log_wbuff, "A sensor node with %d has opened a new connection\n", data.id);
         			write_to_FIFO(log_wbuff,fpwrite);
                }	
            		
			}

			if (server->fd[i+1].revents & POLLRDHUP)
			{
				tcp_close(&(sensor->socket));
				
				#ifdef DEBUG
				printf("Client sensor id %d closed the TCP connection \n",sensor->sensor_id);
				#endif
				
				//write new conn. to FIFO
				asprintf(&log_wbuff, "Client sensor id %d closed the TCP connection\n",(sensor->sensor_id));
         		write_to_FIFO(log_wbuff,fpwrite);
				
				dpl_remove_at_index(connections_to_server,i,false); 
				
				//shift the removed sensor fd as a last element of array
				shift_array(i+1,list_size+1,server->fd);
				server->last_record=time(NULL);
				break;
				
			}
			if(time(NULL)>(sensor->last_record+TIMEOUT))	
			{	
				tcp_close(&(sensor->socket));

				#ifdef DEBUG
				printf("Client sensor id %d excessed the TIMEOUT limit \n",(sensor->sensor_id));
				#endif      
        		
        		//write new conn. to FIFO
        		asprintf(&log_wbuff, "Client sensor id %d excessed the TIMEOUT limit\n",(sensor->sensor_id));
         		write_to_FIFO(log_wbuff,fpwrite);
                 
				dpl_remove_at_index(connections_to_server,i,false); 

				//arrange the poll array to conn.list
				shift_array(i+1,list_size+1,server->fd);
				server->last_record=time(NULL);
				break;
			}
					
		}
	} 
	fclose(fp);		
}

void connmgr_listen(int port_number,sbuffer_t** sbuffer,int* thread_result_connmgr,FILE*fpwrite){
	FILE * fp = fopen("sensor_data_recv","w");
	server_create(port_number);
	if (server)
	{
		manage_connection(fp,sbuffer,thread_result_connmgr,fpwrite);
	}else{
		printf("Can not find proper server\n");
	}
	
}

void shift_array(int index,int size,poll_d* poll_fd){
	for (int i = index; i < size; ++i)
	{
		poll_fd[i]=poll_fd[i+1];

	}	
}

void connmgr_free(){
	dpl_free(&connections_to_server,false);
	free(server->fd);
	free(server);
}