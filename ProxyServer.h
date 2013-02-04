#ifndef LISTEN_H
#define LISTEN_H

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
class ProxyServer {
	public:
		ProxyServer(const char *port);
		void startServer();
	private:
		static const int BACKLOG = 10;
	
		//Initializes addrinfo based on the port.
		struct addrinfo* initAddrInfo(const char *port);
		void reapZombies();
		void* get_in_addr(struct sockaddr *sa);
		int acceptConnection(int listen_fd);
		void handleConnection(int conn_fd);
		int listen_fd;
};
		//Workaround, this function can't be in a class.
		void sigchld_handler(int s);

#endif