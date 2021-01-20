
#include "sensor_db.h"

/*
 *inspired from http://zetcode.com/db/sqlitec/ 
 *SQLite C tutorial section #insert_data.c and #last_row_id.c
 */
  
DBCONN *init_connection(char clear_up_flag,FILE* fpwrite){
   
	sqlite3 *db;
    char *err_msg = 0;

    //open DB
    int rc = sqlite3_open(TO_STRING(DB_NAME), &db);
    char *log_wbuff;
    
    if(rc != SQLITE_OK) {
        
        fprintf(stderr, "Unable to connect to SQL server.: %s\n", sqlite3_errmsg(db));
        //write to FIFO
        asprintf(&log_wbuff, "Unable to connect DB\n");
        write_to_FIFO(log_wbuff,fpwrite);
        
        sqlite3_close(db);
        return NULL;
    }

    //write to FIFO
    asprintf(&log_wbuff, "Connection to SQL server established\n");
    write_to_FIFO(log_wbuff,fpwrite);

    char sql[150]; 

     if(clear_up_flag){
        
        snprintf(sql,150,"DROP TABLE IF EXISTS %s;"
                         "CREATE TABLE %s (id INTEGER,sensor_id INT,sensor_value DECIMAL(4 , 2),timestamp TIMESTAMP,PRIMARY KEY(id));", TO_STRING(TABLE_NAME),TO_STRING(TABLE_NAME));

    }else{
        snprintf(sql,150,"CREATE TABLE IF NOT EXISTS %s (id INTEGER,sensor_id INT,sensor_value DECIMAL(4 , 2),timestamp TIMESTAMP,PRIMARY KEY(id));",TO_STRING(TABLE_NAME));

    }

    //execute the query
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    
    if(rc != SQLITE_OK ) {
        
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);        
     	return NULL;
    }

    asprintf(&log_wbuff, "New table %s created\n",TO_STRING(TABLE_NAME));
    write_to_FIFO(log_wbuff,fpwrite);
    return db;
}


void disconnect(DBCONN *conn,FILE *fpwrite){
    char *log_wbuff;

    asprintf(&log_wbuff, "Connection to SQL server lost\n");
    write_to_FIFO(log_wbuff,fpwrite);

 	sqlite3_close(conn);
}

void get_sbuffer_data(DBCONN *conn, sbuffer_t** sbuffer,int* thread_result_connmgr){
    sensor_data_t * buffer_data;
    buffer_data = malloc(sizeof(sensor_data_t));
    int result=0;

    while(*thread_result_connmgr !=0 || result!=SBUFFER_NO_DATA){
    
        
        result=sbuffer_get_data(*sbuffer,buffer_data);
        if (result==SBUFFER_NO_DATA) 
        {   
            pthread_rwlock_unlock(&(*sbuffer)->lock);
            continue;
        }
        #ifdef DEBUG
        printf("data is read by storagemgr %d sensorid\n",buffer_data->id );
        #endif
        
        insert_sensor(conn, buffer_data->id, buffer_data->value, buffer_data->ts);
        
        #ifdef DEBUG
        printf("sensor %d is inserted db\n",buffer_data->id);
        #endif

        int storage=0;
        sbuffer_remove(*sbuffer,buffer_data,&storage);
        pthread_barrier_wait(&(*sbuffer)->prep_barrier); 
      
    }

    free(buffer_data);

}


int insert_sensor(DBCONN *conn, sensor_id_t id, sensor_value_t value, sensor_ts_t ts){
	
    char *err_msg = 0;
	char sql[100] ;	
    snprintf(sql,100,"INSERT INTO %s(sensor_id,sensor_value,timestamp) VALUES(%hd,%f,%ld)", TO_STRING(TABLE_NAME),id,value,ts);
    
    int rc = sqlite3_exec(conn, sql, 0, 0, &err_msg);

    if(rc != SQLITE_OK ) {
        
         fprintf(stderr, "SQL error: %s\n", err_msg);
         sqlite3_free(err_msg);        
       
     	 return -1;
    }
    return 0;
}

int insert_sensor_from_file(DBCONN *conn, FILE *sensor_data){
	sensor_id_t sensor_bfr;
	sensor_value_t  temperature_bfr;
    sensor_ts_t time_bfr;
    
	while(fread(&sensor_bfr,sizeof(sensor_bfr),1,sensor_data)>0){
		
		fread(&temperature_bfr,sizeof(temperature_bfr),1,sensor_data);
		
		fread(&time_bfr,sizeof(&time_bfr),1,sensor_data);
		
		int result=insert_sensor(conn,sensor_bfr,temperature_bfr,time_bfr);
		if(result==-1)  return -1;
	}
	return 0;
}

int find_sensor_all(DBCONN *conn, callback_t f){

	char *err_msg = 0;
	char sql[100] ;	
    snprintf(sql,100,"SELECT * FROM %s", TO_STRING(TABLE_NAME));
    
    int rc = sqlite3_exec(conn, sql, f, 0, &err_msg);
    if(rc != SQLITE_OK ) {
        
         fprintf(stderr, "SQL error: %s\n", err_msg);
         sqlite3_free(err_msg);        
       
     	 return -1;
    }
   
    return 0;
}

int find_sensor_by_value(DBCONN *conn, sensor_value_t value, callback_t f){
		
	char *err_msg = 0;
	char sql[100] ;	
    snprintf(sql,100,"SELECT * FROM %s WHERE sensor_value IS %f ", TO_STRING(TABLE_NAME),value);
    
    int rc = sqlite3_exec(conn, sql, f, 0, &err_msg);
    if(rc != SQLITE_OK ) {
        
         fprintf(stderr, "SQL error: %s\n", err_msg);
         sqlite3_free(err_msg);        
       
     	 return -1;
    }
   
    return 0;
}

int find_sensor_exceed_value(DBCONN *conn, sensor_value_t value, callback_t f){

	char *err_msg = 0;
	char sql[100] ;	
    snprintf(sql,100,"SELECT * FROM %s WHERE sensor_value > %f ", TO_STRING(TABLE_NAME),value);
    
    int rc = sqlite3_exec(conn, sql, f, 0, &err_msg);
    if(rc != SQLITE_OK ) {
        
         fprintf(stderr, "SQL error: %s\n", err_msg);
         sqlite3_free(err_msg);        
       
     	 return -1;
    }
    return 0;
}

int find_sensor_by_timestamp(DBCONN *conn, sensor_ts_t ts, callback_t f){

	char *err_msg = 0;
	char sql[100] ;	
    snprintf(sql,100,"SELECT * FROM %s WHERE timestamp IS % ld ", TO_STRING(TABLE_NAME),ts);
    
    int rc = sqlite3_exec(conn, sql, f, 0, &err_msg);
    	if(rc != SQLITE_OK ) {
        
         fprintf(stderr, "SQL error: %s\n", err_msg);
         sqlite3_free(err_msg);        
       
     	 return -1;
    }
    return 0;
}

int find_sensor_after_timestamp(DBCONN *conn, sensor_ts_t ts, callback_t f){

	char *err_msg = 0;
	char sql[100] ;	
    snprintf(sql,100,"SELECT * FROM %s WHERE timestamp > % ld ", TO_STRING(TABLE_NAME),ts);
    
    int rc = sqlite3_exec(conn, sql, f, 0, &err_msg);
    	if(rc != SQLITE_OK ) {
        
         fprintf(stderr, "SQL error: %s\n", err_msg);
         sqlite3_free(err_msg);        
       
     	 return -1;
    }
    return 0;
}
