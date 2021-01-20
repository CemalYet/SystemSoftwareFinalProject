/**
 * \author Cemal Yetismis
 */

#ifndef _SBUFFER_H_
#define _SBUFFER_H_

#define _GNU_SOURCE
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>

#define SBUFFER_FAILURE -1
#define SBUFFER_SUCCESS 0
#define SBUFFER_NO_DATA 1

/**
 * basic node for the buffer, these nodes are linked together to create the buffer
 */
typedef struct sbuffer_node {
    struct sbuffer_node *next;  /**< a pointer to the next node*/
    sensor_data_t data;         /**< a structure containing the data */
} sbuffer_node_t;

/**
 * a structure to keep track of the buffer
 */
struct sbuffer {
    sbuffer_node_t *head;       	/**< a pointer to the first node in the buffer */
    sbuffer_node_t *tail; 			/**< a pointer to the last node in the buffer */
    pthread_rwlock_t lock;			/**rw lock sync parameter to make list threads safe**/
    pthread_barrier_t loop_barrier; /**barrirer sync parameter to wait other thread to read data to remove data from buffer**/
	pthread_barrier_t prep_barrier; /**barrirer sync parameter to wait other thread to read data to remove data from buffer**/     
};

typedef struct sbuffer sbuffer_t;

/**
 * Allocates and initializes a new shared buffer
 * \param buffer a double pointer to the buffer that needs to be initialized
 * \return SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occurred
 */
int sbuffer_init(sbuffer_t **buffer);

/**
 *Read data from shared buffer
 *When it reads it locks the shared buffer only against the writers threads
 *After reading unlocks the buffer
 *If there is not any avaliable data in the buffer it returns SBUFFER_NO_DATA
*/

int sbuffer_get_data(sbuffer_t* buffer,sensor_data_t* data);

/**
 * All allocated resources are freed and cleaned up
 * \param buffer a double pointer to the buffer that needs to be freed
 * \return SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occurred
 */
int sbuffer_free(sbuffer_t **buffer);

/**
 * Removes the first sensor data in 'buffer' (at the 'head') and returns this sensor data as '*data'
 * To able to remove data it locks the shared buffer agaisnt to readers and writers thread
 * If 'buffer' is empty, the function doesn't block until new sensor data becomes available but returns SBUFFER_NO_DATA
 * \param buffer a pointer to the buffer that is used
 * \param data a pointer to pre-allocated sensor_data_t space, the data will be copied into this structure. No new memory is allocated for 'data' in this function.
 * \return SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occurred
 */
int sbuffer_remove(sbuffer_t *buffer, sensor_data_t *data,int *who);

/**
 * Inserts the sensor data in 'data' at the end of 'buffer' (at the 'tail')
 * Locks the buffer against writer and reader threads when it inserts the data, then releases the lock.
 * \param buffer a pointer to the buffer that is used
 * \param data a pointer to sensor_data_t data, that will be copied into the buffer
 * \return SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occured
*/
int sbuffer_insert(sbuffer_t *buffer, sensor_data_t *data);

#endif  //_SBUFFER_H_
