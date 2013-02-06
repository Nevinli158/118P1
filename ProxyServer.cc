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
#include "http-request.h"

#include "ProxyServer.h"

/*	initServer creates/binds a socket, and returns the fd associated with the socket.
	port = string containing the port number desired.
*/
ProxyServer::ProxyServer(const char *port){
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
		int* conn_fd = new int[1];
        *conn_fd = acceptConnection(listen_fd);
        if (*conn_fd == -1) {
            perror("accept");
            continue;
        }
		
		reapConnectionList(); // Update the number of current active connections
		
		//Refuse to serve if the server is already serving too many clients.
		if(connectionList.size() < MAX_NUM_CLIENTS){	
			//Fork a child to handle this specific connection
			pthread_t newThread;
			pthread_create(&newThread, NULL, ProxyServer::handleConnection, (void*)conn_fd);
			connectionList.push_back(newThread);
		} else {
			printf("server:but connection refused \n");
			close(*conn_fd);
		}
    }
}

void ProxyServer::reapConnectionList(){
	int status = 0;
	std::list<pthread_t>::iterator it = connectionList.begin();
	//For each connection
	while(it != connectionList.end()){
		status = pthread_tryjoin_np(*it, NULL);
		//If the connection has ended, remove it from the list.
		if(status == 0){
			it = connectionList.erase(it);
		} else if(status == EBUSY){ //If the connection is still going, move on.
			it++;
		} else {//Error
			perror("reap error");
			connectionList.erase(it);
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


void* ProxyServer::handleConnection(void* args){
	/*
	if (send(conn_fd, "Hello, world!", 13, 0) == -1)
                perror("send");
	close(conn_fd);
	exit(0);
	*/
	// recv from socket into buffer
	int conn_fd = *((int*)args);
	size_t buf_size = 1024;
	
	size_t requestbuf_size = 0;
	size_t max_requestbuf_size = buf_size;
	// Full request buffer
	char *requestbuf = (char *) malloc(buf_size);
	// Temp recv buffer
	char *recvbuf = (char *) malloc(buf_size);
		
	int recvbytes = recv(conn_fd, recvbuf, buf_size, 0);
	if (recvbytes == -1) {
		perror("recv");
	}
	else if (recvbytes == 0){
		printf("connection closed\n");
	}
	else {
		HttpRequest *http_request = new HttpRequest();
		while (recvbytes != 0 || recvbytes != -1) {
			if((requestbuf_size + recvbytes) >= max_requestbuf_size) {
				// Grow requestbuf by buf_size
				requestbuf = (char *) realloc(requestbuf, max_requestbuf_size + buf_size);
				max_requestbuf_size += buf_size;
			}
			memcpy(requestbuf + requestbuf_size, recvbuf, recvbytes);
			requestbuf_size += recvbytes;
			try {
				http_request->ParseRequest(requestbuf, requestbuf_size);
			} catch(ParseException e) {
				recvbytes = recv(conn_fd, requestbuf, buf_size, 0);
				continue;
			}
		}
		/*
		// Format request to remote server
		char *remote_request = (char *) malloc(http_request->GetTotalLength());
		int sendbytes = http_request->FormatRequest(remote_request) - remote_request;
		*/
		std::cout << http_request->GetHost() << ", " << http_request->GetPort() << ", " << http_request->GetPath() << std::endl;
		
		/*
		// Connect to remote server
		char * port = new char[6];
		sprintf(port, "%d", http_request->GetPort());
		struct addrinfo *servinfo = initAddrInfo(port, http_request->GetHost().c_str());
		int serverfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
		if (bind(serverfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
            close(serverfd);
            perror("handle: bind");
			exit(-1);
        }
		connect(serverfd, servinfo->ai_addr, servinfo->ai_addrlen);
		
		if (send(serverfd, remote_request, sendbytes, 0) == -1)
                perror("send");
		*/
	}
	
	close(conn_fd);  //close the connection once we're done.
	return NULL;
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
