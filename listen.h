#ifndef LISTEN_H
#define LISTEN_H

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

//Creates a socket for the port. Port is a string containing the port number.
int initServer(const char *port);
//Initializes addrinfo based on the port.
struct addrinfo* initAddrInfo(const char *port);
void reapZombies();
void sigchld_handler(int s);
void *get_in_addr(struct sockaddr *sa);
int acceptConnection(int listen_fd);
int handleConnection(int conn_fd);
#endif