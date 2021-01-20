/**
 * \author Cemal Yetismis
 */

#include "sbuffer.h"

/*
 inspired from pthread Tutorial Peter C. Chapin section 3 
 Thread Synchronization: Barriers p.11 and Reader/Writer Locks p.23
*/

int sbuffer_init(sbuffer_t **buffer) {
    *buffer = malloc(sizeof(sbuffer_t));
    if (*buffer == NULL) return SBUFFER_FAILURE;
    (*buffer)->head = NULL;
    (*buffer)->tail = NULL;

    //rwlock init
    if (pthread_rwlock_init(&(*buffer)->lock, NULL) != 0)
    {
        printf("\n rwlock init is failed\n");
    }
    //barriers init
    if (pthread_barrier_init(&(*buffer)->loop_barrier,NULL,2)!= 0)
    {
        printf("\n loop_barrier init is failed\n");
    }

    if (pthread_barrier_init(&(*buffer)->prep_barrier,NULL,2) !=0)
    {
        printf("\n prep_barrier init is failed\n");
    }
    return SBUFFER_SUCCESS;
}

int sbuffer_get_data(sbuffer_t* buffer,sensor_data_t* data){
     //lock buffer against the writers
    pthread_rwlock_rdlock(&buffer->lock);

    if (buffer == NULL) return SBUFFER_FAILURE;
     if(buffer->head ==NULL)
    {
        return SBUFFER_NO_DATA;
    }
    *data = buffer->head->data;

    pthread_rwlock_unlock(&buffer->lock);
    return SBUFFER_SUCCESS;
}


int sbuffer_free(sbuffer_t **buffer) {
    sbuffer_node_t *dummy;
    if ((buffer == NULL) || (*buffer == NULL)) {
        return SBUFFER_FAILURE;
    }
    while ((*buffer)->head) {
        dummy = (*buffer)->head;
        (*buffer)->head = (*buffer)->head->next;
        free(dummy);
    }
    free(*buffer);
    *buffer = NULL;
    return SBUFFER_SUCCESS;
}

/*
 inspired from pthread Tutorial Peter C. Chapin section 3 
 Thread Synchronization: Barriers p.11 and Reader/Writer Locks p.23
*/

int sbuffer_remove(sbuffer_t *buffer, sensor_data_t *data,int *who) {
    sbuffer_node_t *dummy;
    //check if the other thread reads the data if it is not wait 
     if(pthread_barrier_wait(&buffer->loop_barrier)==PTHREAD_BARRIER_SERIAL_THREAD)
        {   
            //if two threads read the data lock the buffer to remove it
            pthread_rwlock_wrlock(&buffer->lock);
           
            if (buffer == NULL) return SBUFFER_FAILURE;
            if (buffer->head == NULL) return SBUFFER_NO_DATA;
            *data = buffer->head->data;
            dummy = buffer->head;
            if (buffer->head == buffer->tail)
            {
                buffer->head = buffer->tail = NULL;
                
            } else  // buffer has many nodes empty
            {   
            buffer->head = buffer->head->next;
            
            #ifdef DEBUG
            printf("tail has more node\n");
            #endif
            }
             free(dummy);
            //added to check which mgr deletes the node
            if (*who)
            {     
                #ifdef DEBUG
                //printf("removed by datamgr\n");
                #endif

            }else{
                
                #ifdef DEBUG
                //printf("removed by storage manager\n");
                #endif
            }
            pthread_rwlock_unlock(&buffer->lock);
        }
    return SBUFFER_SUCCESS;
}

/*
 inspired from pthread Tutorial Peter C. Chapin section 3 
 Thread Synchronization:Reader/Writer Locks p.23
*/
int sbuffer_insert(sbuffer_t *buffer, sensor_data_t *data) {
    sbuffer_node_t *dummy;
    pthread_rwlock_wrlock(&buffer->lock);
  
     if (buffer == NULL) {

        #ifdef DEBUG
        printf("buffer null\n");
        #endif
        pthread_rwlock_unlock(&buffer->lock);
        return SBUFFER_FAILURE;
    }

    dummy = malloc(sizeof(sbuffer_node_t));
    assert(dummy!=NULL);

    if (dummy == NULL) {

        pthread_rwlock_unlock(&buffer->lock);
        return SBUFFER_FAILURE;
    }
       
    dummy->data = *data;
    dummy->next = NULL;
    if (buffer->tail == NULL) // buffer empty (buffer->head should also be NULL
    {
        buffer->head = buffer->tail = dummy;
    } else // buffer not empty
    {
        buffer->tail->next = dummy;
        buffer->tail = buffer->tail->next;
    }
    pthread_rwlock_unlock(&buffer->lock);
    return SBUFFER_SUCCESS;
}
