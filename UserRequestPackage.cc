#include "UserRequestPackage.h"
#include "http-request.h"

UserRequestPackage::UserRequestPackage(HttpRequest *request, int fd){
	conn_fd = fd;
	request = http_request;
}