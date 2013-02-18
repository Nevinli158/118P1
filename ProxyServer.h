#ifndef PROXY_SERVER_H
#define PROXY_SERVER_H

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <list>
#include <pthread.h>
#include "http-request.h"
class ProxyServer {
	public:
		ProxyServer(const char *port);
		void startServer();
	private:
		static const int BACKLOG = 10;
		static const unsigned int MAX_NUM_CLIENTS = 10; // how many clients the server will serve at any given time
		static const unsigned int MAX_NUM_SINGLE_USER_REQUESTS = 50; // how many requests the server will serve from a single client at any given time
		//Initializes addrinfo based on the port.
		static struct addrinfo* initAddrInfo(const char *port, const char *ip=NULL); 

		void reapZombies();
		void* get_in_addr(struct sockaddr *sa);
		int acceptConnection(int listen_fd);
		static void* handleUserConnection(void* args);
		static void* handleUserRequest(void* args);
		static HttpRequest* getHttpRequest(int conn_fd);
		static void reapThreadList(std::list<pthread_t> *list);
		static int connectToServer(const char* url, unsigned short port);
		int listen_fd;

		std::list<pthread_t> *connectionList;

};
		//Workaround, this function can't be in a class.
		void sigchld_handler(int s);

#endif
