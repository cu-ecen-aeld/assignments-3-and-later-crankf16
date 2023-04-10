// Assignment 5 Part 1
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

#define PORT 9000
#define OUTPUT_FILE "/var/tmp/aesdsocketdata"
#define BUFFER_SIZE 1024

int socket_server;
int socket_client;
int option = 1;

// Handler for SIGNT and SIGTERM from LSP Pg 343
void signal_handler (int signo)
{
	if (signo == SIGINT || signo == SIGTERM)
	{	
		syslog(LOG_INFO, "Caught signal, exiting");
		close(socket_server);
		close(socket_client);
		remove(OUTPUT_FILE);
		exit(1);
	}
}

int main(int argc, char *argv[]) 
{
	// Open the syslog
	openlog("aesdsocket", 0, LOG_USER);

	// Register signal_handler for SIGINT & SIGTERM // LSP Page 343
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	
	// Create socker file descriptor // https://www.geeksforgeeks.org/socket-programming-cc/
	if ((socket_server = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		syslog(LOG_ERR, "Error creating socket server");
	    	exit(-1);
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
	bind(socket_server, (struct sockaddr *)&server_addr, sizeof(server_addr)); 

	// Listen // Beej's 5.5
	listen(socket_server, 10);

	// daemom
	if (argc == 2 && strcmp(argv[1], "-d") == 0) 
	{
		daemon (0, 0); // LSP Page 174
	}

	// Wait and accept
	while (1) 
	{
		struct sockaddr_in client_addr; // Beej's 5.6
	    	socklen_t client_addr_size = sizeof(client_addr);
	    	socket_client = accept(socket_server, (struct sockaddr *)&client_addr, &client_addr_size);

	    	// Connection log as required by the assignment
	   	char client_ip4[INET_ADDRSTRLEN]; // Beej's 6.2
	    	inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip4, INET_ADDRSTRLEN);
	   	syslog(LOG_INFO, "Accepted connection from %s", client_ip4);
	   
		// Open the output file
	    	int out_put = open(OUTPUT_FILE, O_RDWR | O_APPEND | O_CREAT, 0644);
	    
	    	char buf[BUFFER_SIZE]; //Beej's 9.8
	    	int byte_count;
                
		// Receive data
	    	while ((byte_count = recv(socket_client, buf, BUFFER_SIZE - 1, 0)) > 0) 
		{
	    		write(out_put, buf, byte_count);       
			// Find the end of a packet
	    		if (buf[byte_count - 1] == '\n') 
			{
	    			break;
			}
	   	}
	    	
		// Return data
	    	lseek(out_put, 0, SEEK_SET);
	    	while ((byte_count = read(out_put, buf, BUFFER_SIZE)) > 0) 
		{
	    		send(socket_client, buf, byte_count, 0);
	    	}
	 
	    	close(out_put);
	    	close(socket_client);
	}

	return 0;

}
