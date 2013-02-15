#include "UserConnectionPackage.h"
#include "WebCache.h"

UserConnectionPackage::UserConnectionPackage(int fd, WebCache* c){
	conn_fd = fd;
	cache = c;
}