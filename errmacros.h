
/*******************************
This header file is same with 
the errmacros.h in the lectures
*******************************/

#ifndef __errmacros_h__
#define __errmacros_h__

#define MALLOC_MEMORY_ERROR 1

#include <errno.h>

#define SYSCALL_ERROR(err) 									\
		do {												\
			if ( (err) == -1 )								\
			{												\
				perror("Error executing syscall");			\
				exit( EXIT_FAILURE );						\
			}												\
		} while(0)
		
#define CHECK_MKFIFO(err) 									\
		do {												\
			if ( (err) == -1 )								\
			{												\
				if ( errno != EEXIST )						\
				{											\
					perror("Error executing mkfifo");		\
					exit( EXIT_FAILURE );					\
				}											\
			}												\
		} while(0)
		
#define FILE_OPEN_ERROR(fp) 								\
		do {												\
			if ( (fp) == NULL )								\
			{												\
				perror("File open failed");					\
				exit( EXIT_FAILURE );						\
			}												\
		} while(0)

#define FILE_CLOSE_ERROR(err) 								\
		do {												\
			if ( (err) == -1 )								\
			{												\
				perror("File close failed");				\
				exit( EXIT_FAILURE );						\
			}												\
		} while(0)

#define ASPRINTF_ERROR(err) 								\
		do {												\
			if ( (err) == -1 )								\
			{												\
				perror("asprintf failed");					\
				exit( EXIT_FAILURE );						\
			}												\
		} while(0)

#define FFLUSH_ERROR(err) 								\
		do {												\
			if ( (err) == EOF )								\
			{												\
				perror("fflush failed");					\
				exit( EXIT_FAILURE );						\
			}												\
		} while(0)

#define MALLOC_ERR_HANDLER(condition,err_code) \
		do {                  \
			if ((condition)) DEBUG_PRINTF(#condition " failed\n");    \
						assert(!(condition));                                    \
			 } while(0)

#ifdef DEBUG
	#define DEBUG_PRINTF(...)                   \
			do {                    \
			 		fprintf(stderr,"\nIn %s - function %s at line %d: ", __FILE__, __func__, __LINE__);  \
			 		fprintf(stderr,__VA_ARGS__);         \
			 		fflush(stderr);                                                                          \
			 	} while(0)
			#else
	#define DEBUG_PRINTF(...) (void)0
#endif

#endif


