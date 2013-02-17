#include <iostream>
#include <cstdlib>
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h> 
#include <signal.h>
#include <sys/wait.h>
#include <cstring>
#include <arpa/inet.h>
#include <list>
#include <pthread.h>
#include <errno.h>
#include "buffer.h"
#include "http-request.h"
#include "UserConnectionPackage.h"
#include "UserRequestPackage.h"
#include "ProxyServer.h"

/*	initServer creates/binds a socket, and returns the fd associated with the socket.
	port = string containing the port number desired.
*/
ProxyServer::ProxyServer(const char *port){
	connectionList = new std::list<pthread_t>();

	//Load address info struct
	struct addrinfo *servinfo = initAddrInfo(port);
	//Make a socket
	listen_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	if(listen_fd == -1){
		std::cout<<"socket init error\n";
	}
	
	struct addrinfo *p;
    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((listen_fd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(listen_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(listen_fd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        exit(2);
    }	
	
	freeaddrinfo(servinfo);	
}

void ProxyServer::startServer(){
	//Start to listen
	if (listen(listen_fd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
	//Get rid of old zombie threads.
	reapZombies();
	printf("server: waiting for connections...\n");
	
    while(1) {  // main accept() loop
		int conn_fd;
        conn_fd = acceptConnection(listen_fd);
        if (conn_fd == -1) {
            perror("accept");
            continue;
        }
		
		reapThreadList(connectionList); // Update the number of current active connections
		
		//Refuse to serve if the server is already serving too many clients.
		if(connectionList->size() < MAX_NUM_CLIENTS){	
			//Fork a child to handle this specific connection
			pthread_t newThread;
			pthread_create(&newThread, NULL, ProxyServer::handleUserConnection, new UserConnectionPackage(conn_fd, new WebCache()));
			connectionList->push_back(newThread);
		} else {
			printf("server:but connection refused \n");
			close(conn_fd);
		}
    }
}

void ProxyServer::reapThreadList(std::list<pthread_t> *list){
	int status = 0;
	std::list<pthread_t>::iterator it = list->begin();
	//For each connection
	while(it != list->end()){
		status = pthread_tryjoin_np(*it, NULL);
		//If the connection has ended, remove it from the list.
		if(status == 0){
			it = list->erase(it);
		} else if(status == EBUSY){ //If the connection is still going, move on.
			it++;
		} else {//Error
			perror("reap error");
			list->erase(it);
			continue;
		}
	}	
}

/*	initAddrInfo initializes the addrinfo struct based on the port number.
	port = string containing the port number desired.
*/
struct addrinfo* ProxyServer::initAddrInfo(const char *port, const char *host){
	int status;
	struct addrinfo hints;
	struct addrinfo *servinfo;  // will point to the results

	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
	if ((status = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}
	return servinfo;	
}

int ProxyServer::acceptConnection(int listen_fd){
	struct sockaddr_storage their_addr; // connector's address information
	char s[INET6_ADDRSTRLEN];
	socklen_t sin_size = sizeof their_addr;
	int new_fd = accept(listen_fd, (struct sockaddr *)&their_addr, &sin_size);
	if (new_fd == -1) {
		return -1;
	}
	inet_ntop(their_addr.ss_family,
		get_in_addr((struct sockaddr *)&their_addr),
		s, sizeof s);
	printf("server: got connection from %s\n", s);
	return new_fd;
}


void* ProxyServer::handleUserConnection(void* args){

	std::list<pthread_t> *requestList = new std::list<pthread_t>();
	UserConnectionPackage* pack = (UserConnectionPackage*)(args);
	int conn_fd = pack->conn_fd;
	WebCache* cache = pack->cache;
	
	while(1){
		HttpRequest *http_request = getHttpRequest(conn_fd);
		reapThreadList(requestList); // Update the number of current active requests
		if(http_request == NULL){
		//Do we need to do any more error checking here?
			break;
		}
		std::cout << http_request->FindHeader("Host") << ", " << http_request->GetPort() << ", " << http_request->GetPath() << std::endl;
	
		//Refuse to serve if the server is already serving too many requests.
		if(requestList->size() < MAX_NUM_SINGLE_USER_REQUESTS){	
			//Fork a child to handle this specific request
			pthread_t newThread;
			std::cout << "hi" << std::endl;
			pthread_create(&newThread, NULL, ProxyServer::handleUserRequest, new UserRequestPackage(http_request,conn_fd, cache));
			requestList->push_back(newThread);
		} else {
			printf("Too many user requests \n");
		}
	}
	close(conn_fd);  //close the connection once we're done.
	return NULL;
}

void* ProxyServer::handleUserRequest(void* args){
	UserRequestPackage* package = (UserRequestPackage*)args;
	HttpRequest* http_request = package->http_request;
	
	// Format request to remote server
	int sendbytes = http_request->GetTotalLength();
	char *remote_request = (char *) malloc(sendbytes);
	http_request->FormatRequest(remote_request);
	
	for(int i = 0; i < sendbytes; i++) {
		std::cout << remote_request[i];
	}

	
	// Connect to remote server
	char port[6];
	sprintf(port, "%d", http_request->GetPort());
	struct addrinfo *servinfo = initAddrInfo(port, http_request->GetHost().c_str());
	int serverfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	connect(serverfd, servinfo->ai_addr, servinfo->ai_addrlen);

	// Send HEAD request to remote server 
	// Use HEAD request to get Content-Length header to know size of response
	char head_request[1024];
	char *bufLastPos = head_request;
	// Construct HEAD request
	bufLastPos = stpncpy(bufLastPos, "HEAD ", 5);
	bufLastPos = stpncpy(bufLastPos, http_request->GetPath().c_str(), http_request->GetPath().size());
	bufLastPos = stpncpy(bufLastPos, " HTTP/", 6);
	bufLastPos = stpncpy(bufLastPos, http_request->GetVersion().c_str(), http_request->GetVersion().size());
	bufLastPos = stpncpy(bufLastPos, "\r\n", 2);
	bufLastPos = stpncpy(bufLastPos, "\r\n", 2);
	for(int i = 0; i < 1024; i++) {
		std::cout << head_request[i];
	}


	// Send HEAD request
	if (send(serverfd, head_request, bufLastPos - head_request, 0) == -1) {
		close(serverfd);
		perror("handle: header send");
		exit(-1);
	}
	else {
		// Print out HEAD response
		char recvbuf[1024];
		int recvbytes = recv(serverfd, recvbuf, 1024, 0);
		printf("%s\n%d\n", recvbuf, recvbytes);

		// ***UNTESTED CODE***
		// Should parse HEAD response for Content-Length header
		// and get the full GET response

/*		HttpHeaders response_header;
		// Parse header for Content-Length
		response_header.ParseHeaders(recvbuf, recvbytes);
		std::string l = response_header.FindHeader("Content-Length");
		int content_length = atoi(l.c_str());
		// Send full GET request
		if (send(serverfd, remote_request, sendbytes, 0) == -1) {
			close(serverfd);
			perror("handle: send");
			exit(-1);
		}
		else {
			int response_size = response_header.GetTotalLength() + content_length;
			char buffer[response_size];
			int recvbytes = recv(serverfd, buffer, response_size, 0);
			printf("%s\n", buffer);
			std::cout << recvbytes << std::endl;
		}
*/	}


	// ***OLD RESPONSE CODE DO NOT USE***
	/*
	// Send GET request to remote server
	if (send(serverfd, remote_request, sendbytes, 0) == -1) {
		close(serverfd);
		perror("handle: send");
		exit(-1);
	}
	// Construct response from remote server
	else {
		
		
		// Full response buffer asdf
		Buffer *responsebuf = new Buffer();
		// Temp recv buffer
		Buffer *recvbuf = new Buffer();
		int recvbytes = recv(serverfd, recvbuf->buf, recvbuf->maxsize, 0);
		
		HttpResponse *http_response = new HttpResponse();
		try {
			http_response->ParseResponse(responsebuf->buf, responsebuf->maxsize);
		} catch(ParseException e) {
			std::cout << e.what() << std::endl;
			while (recvbytes != 0 && recvbytes != -1) {
				std::cout << "hi" << std::endl;
				responsebuf->add(recvbuf, recvbytes);
				try {
					http_response->ParseResponse(responsebuf->buf, responsebuf->size);
					break;
				} catch(ParseException e) {
					std::cout << e.what() << std::endl;
					printf("%s\n", responsebuf->buf);
					recvbytes = recv(serverfd, recvbuf->buf, recvbuf->maxsize, 0);
					continue;
				}164.67.100.233
				
			}
		}
		if (recvbytes == -1) {
			perror("recv");
		}
		else if (recvbytes == 0){
			printf("connection closed\n");
		}
		HttpRequest* ProxyServer::getHttpRequest(int conn_fd) {
	// Full request buffer

		std::cout << responsebuf << std::endl;
	}*/
	return NULL;
}

HttpRequest* ProxyServer::getHttpRequest(int conn_fd) {
	// Full request buffer
	Buffer *requestbuf = new Buffer();
	// Temp recv buffer
	Buffer *recvbuf = new Buffer();
	int recvbytes = recv(conn_fd, recvbuf->buf, recvbuf->maxsize, 0);
	
	HttpRequest *http_request = new HttpRequest();
	try {
		http_request->ParseRequest(requestbuf->buf, requestbuf->maxsize);
	} catch(ParseException e) {
		std::cout << e.what() << std::endl;
		// Construct HTTP request by appending recv lines
		while (recvbytes != 0 && recvbytes != -1) {
			requestbuf->add(recvbuf, recvbytes);
			try {
				http_request->ParseRequest(requestbuf->buf, requestbuf->size);
				break;
			} catch(ParseException e) {
				std::cout << e.what() << std::endl;
				printf("%s\n", requestbuf->buf);
				recvbuf->clear();
				recvbytes = recv(conn_fd, recvbuf->buf, recvbuf->maxsize, 0);
				continue;
			}
			
		}
	}
	if (recvbytes == -1) {
		perror("recv");
	}
	else if (recvbytes == 0){
		printf("connection closed\n");
	}
	
	if(http_request->GetHost() == "") {
		http_request->SetHost(http_request->FindHeader("Host"));
	}
	if(http_request->GetPort() == 0) {
		http_request->SetPort(80);
	}

	return http_request;
}

void ProxyServer::reapZombies(){
	struct sigaction sa;
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }	
}

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void* ProxyServer::get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
