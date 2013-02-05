#ifndef LISTEN_H
#define LISTEN_H

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <list>
class ProxyServer {
	public:
		ProxyServer(const char *port);
		void startServer();
	private:
		static const int BACKLOG = 10;
		static const unsigned int MAX_NUM_CLIENTS = 10; // how many clients the server will serve at any given time
	
		//Initializes addrinfo based on the port.
		struct addrinfo* initAddrInfo(const char *port, const char *ip=NULL); 
		void reapZombies();
		void* get_in_addr(struct sockaddr *sa);
		int acceptConnection(int listen_fd);
		void handleConnection(int conn_fd);
		void reapConnectionList();
		int listen_fd;
		std::list<int> connectionList;
};
		//Workaround, this function can't be in a class.
		void sigchld_handler(int s);

#endif
/*
private:
	static const int BACKLOG = 10; // how many pending connections queue will hold
	static const unsigned int MAX_NUM_CLIENTS = 10; // how many clients the server will serve at any given time

	//Initializes addrinfo based on the port.
	static struct addrinfo* initAddrInfo(const char *port);
	void reapZombies();
	void* get_in_addr(struct sockaddr *sa);
	int acceptConnection(int listen_fd);
	static void* handleConnection(void* conn_fd);
	void reapConnectionList();
	int listen_fd;
	std::list<pthread_t> connectionList;
*/
