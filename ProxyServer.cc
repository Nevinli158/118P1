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
#include "http-response.h"
#include "UserConnectionPackage.h"
#include "UserRequestPackage.h"
#include "ProxyServer.h"

/*	The constructor creates/binds a socket, and returns the fd associated with the socket.
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

	// Send full GET request
	if (send(serverfd, remote_request, sendbytes, 0) == -1) {
		close(serverfd);
		perror("handle: send server request");
		exit(-1);
	}
	else {
		char c;
		Buffer *response = new Buffer();
		while(response->size < 4 || 
				strstr(response->buf + response->size - 4, "\r\n\r\n") == NULL) {
			if(recv(serverfd, &c, 1, 0) != -1) {
				response->add(c);
			}
			else {
				std::cout << "recv error: headers" << std::endl;
				break; // TODO: error if connection unexpectedly closes
			}
		}
			
		HttpResponse *http_response = new HttpResponse();
		http_response->ParseResponse(response->buf, response->size);
		std::string l = http_response->FindHeader("Content-Length");
		if(l == "") {
			// TODO: no content length error
			std::cout << "no content length" << std::endl;
		}
		int content_length = atoi(l.c_str());
		int count = 0;
		char *message_body = (char *) calloc(content_length, 1);
		while(count < content_length) {
			int recvbytes = recv(serverfd, message_body + count, content_length - count, 0);
			if(recvbytes == -1) {
				// TODO: error if connection unexpectedly closes
				std::cout << "recv error: message body" << std::endl;
			}
			count += recvbytes;
		}
		
		response->add(message_body, content_length);
		response->print();
		
		try {
			http_response->ParseResponse(response->buf, response->size);
		} catch (ParseException e) {
			std::cout << e.what() << std::endl;
			// TODO: error
		}
		
		if (send(package->conn_fd, response->buf, response->size, 0) == -1) {
			close(package->conn_fd);
			perror("handle: send client response");
			exit(-1);
		}
	}

	return NULL;
}

HttpRequest* ProxyServer::getHttpRequest(int conn_fd) {
	char c;
	Buffer *request	= new Buffer();
	while(request->size < 4 || 
			strstr(request->buf + request->size - 4, "\r\n\r\n") == NULL) {
		if(recv(conn_fd, &c, 1, 0) != -1) {
			request->add(c);
		}
		else {
			std::cout << "recv error: headers" << std::endl;
			break; // TODO: error if connection unexpectedly closes
		}
	}
	
	HttpRequest *http_request = new HttpRequest();
	try {
		http_request->ParseRequest(request->buf, request->size);
	} catch (ParseException e) {
		HttpResponse badrequest;
		badrequest.SetVersion(http_request->GetVersion());
		badrequest.SetStatusCode("400");
		badrequest.SetStatusMsg("Bad request");
		
		char *buffer = (char *) calloc(badrequest.GetTotalLength(), 1);
		badrequest.FormatResponse(buffer);
		
		send(conn_fd, buffer, badrequest.GetTotalLength(), 0);
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
