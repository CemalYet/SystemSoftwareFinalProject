/**
 * \author Cemal Yetismis
 */

#ifndef DATAMGR_H_
#define DATAMGR_H_


#define _GNU_SOURCE
#include <stdio.h>
#include "config.h"
#include "sbuffer.h"
#include "main.h"
#include "datamgr.h"
#include "lib/dplist.h"


#define NO_ERROR "c"
#define MEMORY_ERROR "b" // error due to mem alloc failure
#define INVALID_ERROR "a"

#ifndef RUN_AVG_LENGTH
#define RUN_AVG_LENGTH 5
#endif

#ifndef SET_MAX_TEMP
#error SET_MAX_TEMP not set
#endif

#ifndef SET_MIN_TEMP
#error SET_MIN_TEMP not set
#endif


//sensor object
typedef struct {
        sensor_id_t sensor_id;
        sensor_id_t room_id;
        sensor_value_t running_avg;
        sensor_ts_t last_modified;
        dplist_t* temperatures;
}sensor_t;

//call-back functions for 
void* sensor_copy(void * sensor);
void sensor_free(void ** sensor);
int sensor_compare(void * sensor1, void * sensor2);

/*
 * Use ERROR_HANDLER() for handling memory allocation problems, invalid sensor IDs, non-existing files, etc.
 */
#define ERROR_HANDLER(condition, ...)    do {                       \
                      if (condition) {                              \
                        printf("\nError: in %s - function %s at line %d: %s\n", __FILE__, __func__, __LINE__, __VA_ARGS__); \
                        exit(EXIT_FAILURE);                         \
                      }                                             \
                    } while(0)

/**
 *  This method holds the core functionality of your datamgr. It takes in 2 file pointers. 
 *  Get data from shared buffer read it and calculates the runnin avg for internal decision making
 *  Remove data from shared buffer if it was read by also storagemgr 
 *  Write messages to FIFO
 */
void datamgr_parse_sensor_data(FILE *fp_sensor_map,sbuffer_t** sbuffer, int* thread_result_connmgr,FILE *fpwrite);
/**
 * This method should be called to clean up the datamgr, and to free all used memory. 
 * After this, any call to datamgr_get_room_id, datamgr_get_avg, datamgr_get_last_modified or datamgr_get_total_sensors will not return a valid result
 */
void datamgr_free(dplist_t* sensorList);

/*
 *finish datamgr when 3 tries to connect DB failed
 */
void finish_datamgr(sbuffer_t** sbuffer,int* thread_result_connmgr);

/**
 * Gets the room ID for a certain sensor ID
 * Use ERROR_HANDLER() if sensor_id is invalid
 * \param sensor_id the sensor id to look for
 * \return the corresponding room id
 */
uint16_t datamgr_get_room_id(sensor_id_t sensor_id,dplist_t* sensorList);

/**
 * Gets the running AVG of a certain senor ID (if less then RUN_AVG_LENGTH measurements are recorded the avg is 0)
 * Use ERROR_HANDLER() if sensor_id is invalid
 * \param sensor_id the sensor id to look for
 * \return the running AVG of the given sensor
 */
sensor_value_t datamgr_get_avg(sensor_id_t sensor_id,dplist_t* sensorList);

/**
 * Returns the time of the last reading for a certain sensor ID
 * Use ERROR_HANDLER() if sensor_id is invalid
 * \param sensor_id the sensor id to look for
 * \return the last modified timestamp for the given sensor
 */
time_t datamgr_get_last_modified(sensor_id_t sensor_id,dplist_t* sensorList);

/**
 *  Return the total amount of unique sensor ID's recorded by the datamgr
 *  \return the total amount of sensors
 */
int datamgr_get_total_sensors(dplist_t* sensorList);

void print_sensor_list(dplist_t* sensorList);

#endif  //DATAMGR_H_
