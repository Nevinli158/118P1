#include "listen.h"
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

/*	initServer creates/binds a socket, and returns the fd associated with the socket.
	port = string containing the port number desired.
*/
int initServer(const char *port){
	//Load address info struct
	struct addrinfo *servinfo = initAddrInfo(port);
	//Make a socket
	int socketFD = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	if(socketFD == -1){
		cout<<"socket init error\n";
	}
	
    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }	
	
	freeaddrinfo(servinfo);	
	return socketFD;
}

/*	initAddrInfo initializes the addrinfo struct based on the port number.
	port = string containing the port number desired.
*/
struct addrinfo* initAddrInfo(const char *port){
	int status;
	struct addrinfo hints;
	struct addrinfo *servinfo;  // will point to the results

	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
	if ((status = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}
	return servinfo;	
}

void acceptConnection(int listen_fd){
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size = sizeof their_addr;
	int new_fd = accept(listen_fd, (struct sockaddr *)&their_addr, &sin_size);
	if (new_fd == -1) {
		return -1;
	}
	inet_ntop(their_addr.ss_family,
		get_in_addr((struct sockaddr *)&their_addr),
		s, sizeof s);
	printf("server: got connection from %s\n", s);
}

void handleConnection(int conn_fd){
	if (send(conn_fd, "Hello, world!", 13, 0) == -1)
                perror("send");
	close(conn_fd);
	exit(0);
}

void reapZombies(){
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
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
