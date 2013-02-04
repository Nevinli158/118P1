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

#define MYPORT "14820"  // the port users will be connecting to
#define BACKLOG 10     // how many pending connections queue will hold

int main (int argc, char *argv[])
{
	/* Listening - Establish a socket connection for listening to incoming connections  (port 14805)
		Proxy should run with no more than 10 processes, serves at most 10 incoming connections	
		Once client connected, it should check for proper HTTP request.
		TODO: Need to handle forking max of 10 threads 
	*/
	ProxyServer proxy(MYPORT);	
	proxy.startServer();

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
