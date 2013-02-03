/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <iostream>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h> 
#include "listen.h"

using namespace std;

#define MYPORT "14805"  // the port users will be connecting to
#define BACKLOG 10     // how many pending connections queue will hold

int main (int argc, char *argv[])
{
	/* Listening - Establish a socket connection for listening to incoming connections  (port 14805)
		Proxy should run with no more than 10 processes, serves at most 10 incoming connections	
		Once client connected, it should check for proper HTTP request.
		TODO: Need to handle forking max of 10 threads 
	*/
	int listen_fd = initServer(MYPORT);
	if (listen(listen_fd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
	//Get rid of old zombie threads.
	reapZombies();
	
printf("server: waiting for connections...\n");

	int conn_fd;
    while(1) {  // main accept() loop
        conn_fd = acceptConnection(listen_fd);
        if (conn_fd == -1) {
            perror("accept");
            continue;
        }
		//Fork a child to handle this specific connection
        if (fork() == 0) { // this is the child process
            close(listen_fd); // child doesn't need the listener
			handleConnection(conn_fd);
        }
		
        close(conn_fd);  // parent doesn't need this
    }

    return 0;	
	
	/* Parsing URL
		Parse the requested URL - 3 things: requested host, port, path
	*/

	/* Getting Data from the Remote Server
		Make a connection to the request host and send the request.
	*/

	/* Returning Data to the Client

	*/

	/* Pipeline
		Proxy supports persistent connection.
	*/

	/* Caching
		Cache web pages. Implement conditional GET
	*/
  return 0;
}
