#include "UserRequestPackage.h"
#include "http-request.h"
#include "WebCache.h"

UserRequestPackage::UserRequestPackage(HttpRequest *request, int fd, WebCache* c){
	conn_fd = fd;
	request = http_request;
	cache = c;
}