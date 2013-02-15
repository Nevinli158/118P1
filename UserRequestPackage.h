#ifndef USER_REQUEST_PACKAGE_H
#define USER_REQUEST_PACKAGE_H
#include "http-request.h"
#include "WebCache.h"

class UserRequestPackage{
	public:
		UserRequestPackage(HttpRequest *request, int fd, WebCache* c);
		int conn_fd;
		HttpRequest *http_request;
		WebCache* cache;
};

#endif