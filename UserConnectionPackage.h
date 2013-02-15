#ifndef USER_CONNECTION_PACKAGE_H
#define USER_CONNECTION_PACKAGE_H
#include "WebCache.h"

class UserConnectionPackage{
	public:
		UserConnectionPackage(int fd, WebCache* c);
		int conn_fd;
		WebCache* cache;
};

#endif