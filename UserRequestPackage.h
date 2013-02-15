#include "http-request.h"

class UserRequestPackage{
	public:
		UserRequestPackage(HttpRequest *request, int fd);
		int conn_fd;
		HttpRequest *http_request;
};