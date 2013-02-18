#include "WebCache.h"

#include <map>
#include <string>
#include <ctime>
#include <cstdio>
#include "ReadWriteLock.h"
#include "http-request.h"
#include "buffer.h"

WebCache::WebCache(){
	
}

/*
	Adds the given page to the cache.
	expTime has a default value of "".
*/
void WebCache::cachePage(const HttpRequest* httpRequest, Buffer* page, const std::string expTime){
	//Do not cache the page if an expiration time wasn't specified.
	if(expTime == ""){
		return;
	}
	
	std::string request = parseRequestKey(httpRequest);

	m_cacheLock.lockForWrite(); 
	m_cache.insert(std::pair<std::string,CachedPage>(request, CachedPage(page, expTime)));
	m_cacheLock.unlock();
}

/*
	Checks the cache for the given request. Returns 
	the cached page if it was found, and NULL if it wasn't.
*/
Buffer* WebCache::checkCache(const HttpRequest* httpRequest){
	std::map<std::string, CachedPage>::iterator it;
	
	std::string request = parseRequestKey(httpRequest);
	
	m_cacheLock.lockForRead(); 
	it = m_cache.find(request);
	bool found = !(it == m_cache.end());
	m_cacheLock.unlock();
	
	//If the request was found in the cache
	if(found){
		//If the page is expired, erase it.
		if(isPageExpired(it->second)){
			m_cacheLock.lockForWrite(); 
			m_cache.erase(it);
			m_cacheLock.unlock();
		} else { //Otherwise return the cached page.
			return (it->second).m_page;
		}
	}
	//TODO: check later
	return NULL;
}


std::string WebCache::parseRequestKey(const HttpRequest* httpRequest){
	char port[6];
	sprintf(port, "%d", httpRequest->GetPort());
	std::string portString(port);
	return httpRequest->GetHost() + ":" + portString + httpRequest->GetPath();
}


/*Returns true if the cached page expired, based on the 
	current system time.
*/
bool WebCache::isPageExpired(const CachedPage page){
	time_t currentTime;
	time(&currentTime);
	return (difftime(page.m_expireTime, currentTime) < 0);
}



WebCache::CachedPage::CachedPage(Buffer* page, const std::string expTime){
	m_page = page;
	tm* t = new tm(); 
	strptime(expTime.c_str(), "%a, %d %b %Y %H:%M:%S",t);
	m_expireTime = mktime(t);
}