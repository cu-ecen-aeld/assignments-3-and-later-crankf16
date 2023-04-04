#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    // struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    usleep(thread_func_args->obtain_mutex); // Linux manual https://man7.org/linux/man-pages/man3/usleep.3.html

    int rc = pthread_mutex_lock(thread_func_args->mutex);  // Synchronization Slide 10
    if (rc != 0)
    {
	printf("Mutex lock failed with %d\n", rc);
    }

    usleep(thread_func_args->release_mutex); // Linux manual https://man7.org/linux/man-pages/man3/usleep.3.html

    rc = pthread_mutex_unlock(thread_func_args->mutex);  // Synchronization Slide 10
    if (rc != 0)
    {
    	printf("Mutex unlock failed with %d\n", rc);
    }

    thread_func_args->thread_complete_success = true;
	
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

     struct thread_data *data = (struct thread_data *)malloc(sizeof(struct thread_data));
     // Dynamic memory allocation page 296
     if (data == NULL)
     {
     	perror("Memory error");
        return false;
     }

     data->mutex = mutex;
     data->obtain_mutex = wait_to_obtain_ms;
     data->release_mutex = wait_to_release_ms; 

     int ret = pthread_create(thread, NULL, threadfunc, data); // Synchronization Slide 12
     if (ret != 0)
     {
     	perror("Create new thread failed");
	return false;
     }	    
	
     return true;
}

