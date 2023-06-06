// Assignment 6 Part 1
// Reworked version of Assignment 5 Part 1
// Added acceptance of multiple connections with each one hosted in
//    a seperate thread. Added a timestamp every 10 seconds which is
//    hosted in its own thread.
// References code from:
// Linux System Programming
// https://www.geeksforgeeks.org/socket-programming-cc/
// https://beej.us/guide/bgnet/html/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/queue.h> 

#define PORT 9000
#define OUTPUT_FILE "/var/tmp/aesdsocketdata"
//#define OUTPUT_FILE "/dev/aesdchar"
#define BUFFER_SIZE 1994

int socket_server;
int socket_client;
int option = 1;

typedef struct thread_data
{
  	pthread_mutex_t *mutex;
    	int socket_client;
    	char *client_ip4;
    	pthread_t id;
} 	thread_data;

// From Threading and Linked Lists Slide 10
typedef struct slist_data_s slist_data_t;
slist_data_t *datap = NULL;
SLIST_HEAD(slisthead, slist_data_s) head;

struct slist_data_s
{
    	thread_data thread_param;
    	SLIST_ENTRY(slist_data_s) entry;
};

bool cleanShutdown = false;

// Handler for SIGNT and SIGTERM from LSP Pg 343
void signal_handler(int signo)
{
        syslog(LOG_INFO, "Caught signal, exiting");
        // Close the listening socket
        shutdown(socket_server, SHUT_RDWR);
        cleanShutdown = true; 
}

void *threadFunc(void *thread_param)
{
    	int byte_count;          	// number of bytes received
    	int byte_count_out;
    	char buf[BUFFER_SIZE]; 	// incoming buffer
    	thread_data *thread_args = (thread_data *)thread_param;

    	int out_put = open(OUTPUT_FILE, O_RDWR | O_APPEND | O_CREAT, 0644);

    	// Mutex lock
//    	pthread_mutex_lock(thread_args->mutex);
    	lseek(out_put, 0, SEEK_END);

    	// Receive the string
    	while ((byte_count = recv(thread_args->socket_client, buf, BUFFER_SIZE - 1, 0)) > 0)
    	{
		syslog(LOG_INFO, "Read: %s", buf);
		write(out_put, buf, byte_count);       
		// Find the end of a packet
		if (buf[byte_count - 1] == '\n') 
		{
			break;
		}
    	}
    	
    	// Mutex unlock
//    	pthread_mutex_unlock(thread_args->mutex);

    	// Write the string back
    	lseek(out_put, 0, SEEK_SET);
    	
    	syslog(LOG_INFO, "Pre While Read");
    	
    	while ((byte_count_out = read(out_put, buf, BUFFER_SIZE)) > 0) 
    	{
		syslog(LOG_INFO, "Made it here");
		int dang = send(thread_args->socket_client, buf, byte_count_out, 0);
		if (byte_count_out == -1)
		{
		syslog(LOG_ERR, "Error reading");
		}
		else
		{
		syslog(LOG_INFO, "Reading Success");
		}
		if (dang == -1)
		{
		syslog(LOG_ERR, "Error sending");
		}
		else
		{
		syslog(LOG_INFO, "Sending:%s!", buf);
		}
		
    	}
    	
    	close(out_put);
    	
    	// Mutex unlock
    	//pthread_mutex_unlock(thread_args->mutex);
    	
    	close(thread_args->socket_client);

    	syslog(LOG_INFO, "Closed connection from %s", thread_args->client_ip4);
    	
    	return thread_args;
}

void *timestamp(void *mutex)
{
    	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    	
    	while (cleanShutdown == false)
    	{    	
    		sleep(10);
    		time_t t = time(NULL);
    		struct tm *tm = localtime(&t);
    		char buf[94];

    		strftime(buf, sizeof(buf), "timestamp:%a, %d %b %y %T %z\n", tm);
    		
    		// Lock mutex for time
//    		pthread_mutex_lock(mutex);

    		// Write time to file
//    		int out_put = open(OUTPUT_FILE, O_RDWR | O_APPEND | O_CREAT, 0644);

//    		write(out_put, buf, strlen(buf));   	
//    		close(out_put);
    	
    		// Unlock Mutex for time
//    		pthread_mutex_unlock(mutex);
    		
    	}
    	return mutex;
}

int main(int argc, char **argv)
{
    	int socket_client; // New connection
    	char client_ip4[INET_ADDRSTRLEN];
    	
    	pthread_mutex_t mutex;
    	pthread_mutex_init(&mutex, NULL);

	// Daemon setup
	if (argc == 2 && strcmp(argv[1], "-d") == 0) 
	{
		daemon (0, 0); // LSP Page 174
	}

	// Create socket file descriptor 
	// https://www.geeksforgeeks.org/socket-programming-cc/
	if ((socket_server = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		syslog(LOG_ERR, "Error creating socket server");
	}
	
    	// Code from Assignment 5 tips
	setsockopt(socket_server, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

	// Bind server to the port // Beej's "old way"
	struct sockaddr_in server_addr = 
	{
		.sin_family = AF_INET,
		.sin_addr.s_addr = INADDR_ANY,
		.sin_port = htons(PORT)
	};
	
	// Beej's 5.3
	if (bind(socket_server, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		syslog(LOG_ERR, "Error with bind");
	} 

	// Register signal_handler for SIGINT & SIGTERM // LSP Page 343
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);


    	// From Threading and Linked Lists Slide 10
    	SLIST_INIT(&head);

  	// Start timer thread
  	pthread_t tsPthreadId;
    	pthread_create(&tsPthreadId, NULL, timestamp, &mutex);
    	
    	// Beej's 5.6
    	struct sockaddr_in client_addr; 
	socklen_t client_addr_size = sizeof(client_addr);
                   		 
    	while (cleanShutdown == false)
    	{
    		// Listen // Beej's 5.5
		listen(socket_server, 10);
    		
    		// Beej's 5.6
    		if ((socket_client = accept(socket_server, (struct sockaddr *)&client_addr,
        	    &client_addr_size)) < 0)
        	    {
        	    	syslog(LOG_ERR, "Problem accepting");
        	    }
        	    else
        	    {
        	    	syslog(LOG_INFO, "No problem accepting");
        	    }
        	    
       	// Beej's 6.2 	    
        	inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip4, INET_ADDRSTRLEN);        
        	syslog(LOG_INFO, "Accepted connection from %s", client_ip4);
              
		// Track the threads
		datap = (slist_data_t*) malloc(sizeof(slist_data_t));
		SLIST_INSERT_HEAD(&head, datap, entry);
            	datap->thread_param.socket_client = socket_client;
		datap->thread_param.mutex = &mutex;

		// Create new threads
		pthread_create(&(datap->thread_param.id),	
			NULL,	
			threadFunc,	
			&datap->thread_param);
           	
           	// Join old threads
		SLIST_FOREACH(datap, &head, entry)
		{
			pthread_join(datap->thread_param.id, NULL);
			SLIST_REMOVE(&head, datap, slist_data_s, entry);
			free(datap);
			break;
		}
    }

    	while (cleanShutdown)
    	{
        	close(socket_server);
        	// Close out timer thread
        	pthread_cancel(tsPthreadId);
        	pthread_join(tsPthreadId, EXIT_SUCCESS);
        	remove(OUTPUT_FILE);
        	exit(0);
    	}
}
