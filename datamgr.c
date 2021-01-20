
#include "datamgr.h"

void datamgr_parse_sensor_data(FILE *fp_sensor_map,sbuffer_t** sbuffer, int* thread_result_connmgr,FILE *fpwrite){

	char* log_wbuff;
	sensor_id_t sensor_bfr;
	sensor_id_t room_bfr;
	sensor_data_t *buffer_data;
	dplist_t* sensorList;

	//create dp_list for sensors//
	sensorList=dpl_create(sensor_copy,sensor_free,sensor_compare);

	//read sensor data from file
	while (fscanf(fp_sensor_map,"%hd %hd",&room_bfr,&sensor_bfr)>0){
	 	
	 	sensor_t *sensor=(sensor_t*)malloc(sizeof(sensor_t));
	 	MALLOC_ERR_HANDLER(sensor==NULL,MALLOC_MEMORY_ERROR);
	 	
	 	sensor->sensor_id=sensor_bfr;
	 	sensor->room_id=room_bfr;

	 	//create dp_list for temperatures measurement
	 	sensor->temperatures=dpl_create(sensor_copy,sensor_free,sensor_compare);
	 	sensorList=dpl_insert_at_index(sensorList,sensor,150,false);
	}	
	 	int index_of_sensor;
	 	int result=0;

	 	//intialize shared buffer
        buffer_data = malloc(sizeof(sensor_data_t));
        MALLOC_ERR_HANDLER(buffer_data==NULL,MALLOC_MEMORY_ERROR);
      
	 	//check the shared buffer if there is data and server is starting
	 	while(*thread_result_connmgr !=0 || result!=SBUFFER_NO_DATA){
	 		//fill the buffer_data field from shared buffer
	 		result=sbuffer_get_data(*sbuffer,buffer_data);
	 		
	 		if (result==SBUFFER_NO_DATA) 
	 		{	
	 			pthread_rwlock_unlock(&(*sbuffer)->lock);
	 			 continue;
	 		}
	 		//get the sensor id from the buffer and match it with sensorList
	 		sensor_t *sensor;
	 		index_of_sensor=dpl_get_index_of_element(sensorList,(void *)&buffer_data->id);
	 		
	 		//chechk if shared buffer sensor is in the sensorList
	 		if(index_of_sensor>=0){

	 			sensor=(sensor_t*)dpl_get_element_at_index(sensorList,index_of_sensor);

	 			double *added_element=(double *)malloc(sizeof(double));
	 			assert(added_element!=NULL);

	 			//obtain sensor value and insert it in temperatures list as a first element
	 			*added_element=buffer_data->value;
	 			sensor->temperatures=dpl_insert_at_index(sensor->temperatures,(void *)added_element,-1,false);

	 			int size_temp=dpl_size(sensor->temperatures);

	 			//get first element in the temperaturesList
	 			double *first_element=(double *)dpl_get_element_at_index(sensor->temperatures,0);
	 			
	 			//calculate the running average 
	 			if (size_temp<=RUN_AVG_LENGTH){
	 				//add new temperature value in the begining temperatures list
	 				sensor->running_avg=(sensor->running_avg*(size_temp-1)+(*first_element))/size_temp;
	 			
	 			}
	 			else{
	 				//get the most previous value of the temperature list
	 				double *remove=(double *)dpl_get_element_at_index(sensor->temperatures,RUN_AVG_LENGTH);
	 				//calculate average value with new temperature value
	 				sensor->running_avg=((((double)size_temp-1)*sensor->running_avg)-(*remove)+(*first_element))/((double)size_temp-1);
	 				//remove the most previous value of the temperature list
	 				dpl_remove_at_index(sensor->temperatures,RUN_AVG_LENGTH,false);
	 			}

	 			//set time 
	 			sensor->last_modified=buffer_data->ts;
	 			
	 			#ifdef DEBUG
	 			printf("data is read by DATAMGR %d sensorid\n",buffer_data->id );
	 			#endif
	 			
	 			if (sensor->running_avg<SET_MIN_TEMP)
	 			{
	 				#ifdef DEBUG
	 				fprintf( stderr,"It's too cold in room %hu for sensor %hu with a running average of %lf on %ld\n",(sensor->room_id),buffer_data->id,sensor->running_avg,buffer_data->ts);
	 				#endif
	 				asprintf( &log_wbuff, "The sensor node with %hu reports it’s too cold (running avg temperature = %lf) \n", buffer_data->id,sensor->running_avg);
            		write_to_FIFO(log_wbuff,fpwrite);
            		
	 			}
	 			else if(sensor->running_avg > SET_MAX_TEMP)
            	{	
            		#ifdef DEBUG
            	    fprintf( stderr, "It's too warm in room %hu for sensor %hu with a running average of %lf on %ld \n",(sensor->room_id),buffer_data->id,sensor->running_avg,buffer_data->ts);
            		#endif 
    				asprintf( &log_wbuff, "It's too warm in room %hu for sensor %hu with a running average of %lf on %ld \n",(sensor->room_id),buffer_data->id,sensor->running_avg,buffer_data->ts);
    				write_to_FIFO(log_wbuff,fpwrite);
            		
	        	}	
	        		//datamgr value  is set to determine which manager removed data from shared buffer is written for debugging
	        		int datamgr=1;
            		sbuffer_remove(*sbuffer,buffer_data,&datamgr);
	        		pthread_barrier_wait(&(*sbuffer)-> prep_barrier); 
			}
			else {

				//if coming sensor is not in the file
	 			int datamgr=1;
    			asprintf( &log_wbuff, "Received sensor data with invalid sensor node ID %d \n", buffer_data->id);
    			write_to_FIFO(log_wbuff,fpwrite);

    			//delete coming invalid sensor from shared buffer
        		sbuffer_remove(*sbuffer,buffer_data,&datamgr);
        		pthread_barrier_wait(&(*sbuffer)-> prep_barrier); 
	 		}
		}

 	free(buffer_data);
 	datamgr_free(sensorList);
}

void datamgr_free(dplist_t* sensorList){
	
	dpl_free(&sensorList,true);
}

// finish datamgr when 3 tries to connect DB failed
void finish_datamgr(sbuffer_t** sbuffer,int* thread_result_connmgr){
    
    sensor_data_t * buffer_data;
    int datamgr=1;
    int result=0; 
    buffer_data = malloc(sizeof(sensor_data_t));
    assert(buffer_data!=NULL);

    while(*thread_result_connmgr !=0 || result!=SBUFFER_NO_DATA){
        result=sbuffer_get_data(*sbuffer,buffer_data);
        if (result==SBUFFER_NO_DATA) 
        {   
            pthread_rwlock_unlock(&(*sbuffer)->lock);
            continue;
        }
        sbuffer_remove(*sbuffer,buffer_data,&datamgr);
        pthread_barrier_wait(&(*sbuffer)->prep_barrier);   
    }
    free(buffer_data);
}

uint16_t datamgr_get_room_id(sensor_id_t sensor_id,dplist_t* sensorList){
	sensor_t *desired_sensor;
	int index=0;
	while(index<dpl_size(sensorList)){
		desired_sensor=(sensor_t*)dpl_get_element_at_index(sensorList,index);
		if(desired_sensor->sensor_id==sensor_id) return desired_sensor->room_id;	
		index++;
	}
	ERROR_HANDLER(1,INVALID_ERROR);
}

sensor_value_t datamgr_get_avg(sensor_id_t sensor_id,dplist_t* sensorList){
	sensor_t *desired_sensor;
	int index=0;
	while(index<dpl_size(sensorList)){
		desired_sensor=(sensor_t*)dpl_get_element_at_index(sensorList,index);
		if(desired_sensor->sensor_id==sensor_id) return desired_sensor->running_avg;	
		index++;
	}
	ERROR_HANDLER(1,INVALID_ERROR);
}

time_t datamgr_get_last_modified(sensor_id_t sensor_id,dplist_t* sensorList){

	sensor_t *desired_sensor;
	int index=0;
	while(index<dpl_size(sensorList)){
		desired_sensor=(sensor_t*)dpl_get_element_at_index(sensorList,index);
		if(desired_sensor->sensor_id==sensor_id) return desired_sensor->last_modified;
		index++;
	}
	ERROR_HANDLER(1,INVALID_ERROR);
}

int datamgr_get_total_sensors(dplist_t* sensorList){
	return dpl_size(sensorList);

}

void print_sensor_list(dplist_t* sensorList){
	for (int i = 0; i < dpl_size(sensorList); ++i)
	{	
		sensor_t *sensor=dpl_get_element_at_index(sensorList,i);
		printf("sensor id = %d----> room_id=%d----> avg=%f\n",sensor->sensor_id,sensor->room_id,sensor->running_avg);
		printf("########PRINTING TEMPERATURES####################\n");
		for (int j = 0; j <dpl_size(sensor->temperatures) ; ++j)
	 		{
	 			printf(" %f\n", *(double*)dpl_get_element_at_index(sensor->temperatures,j));
	 		}
	}
}

//calback functions
int sensor_compare(void * sensor1, void * sensor2){
	return ((((sensor_t*)sensor1)->sensor_id < ((sensor_t*)sensor2)->sensor_id) ? -1 
		: (((sensor_t*)sensor1)->sensor_id == ((sensor_t*)sensor2)->sensor_id) ? 0 : 1);
}


void sensor_free(void ** sensor){
	dpl_free(&(((sensor_t*)*sensor)->temperatures),false);
    free(*sensor);
    *sensor = NULL;

}

void * sensor_copy(void * sensor) {
    sensor_t* copy = malloc(sizeof (sensor_t));
    assert(copy != NULL);
    copy->sensor_id = ((sensor_t*)sensor)->sensor_id;
    copy->room_id=((sensor_t*)sensor)->room_id;   
    copy->last_modified=((sensor_t*)sensor)->last_modified; 
    copy->temperatures=dpl_create(sensor_copy,sensor_free,sensor_compare);
    for (int i = 0; i < RUN_AVG_LENGTH; ++i)
    {
    	 dpl_insert_at_index(copy->temperatures,dpl_get_element_at_index(((sensor_t*)sensor)->temperatures,i),i,false);
    }
    
    return (void *) copy;
}
        