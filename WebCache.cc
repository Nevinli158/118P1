#include "WebCache.h"

#include <map>
#include <string>
#include <ctime>
#include "ReadWriteLock.h"

WebCache::WebCache(){
	
}

/*
	Adds the given page to the cache.
*/
void WebCache::cachePage(std::string httpRequest, std::string page, std::string expTime){
	m_cacheLock.lockForWrite(); 
	m_cache.insert(std::pair<std::string,CachedPage>(httpRequest, CachedPage(page, expTime)));
	m_cacheLock.unlock();
}

/*
	Checks the cache for the given request. Returns 
	the cached page if it was found, and NULL if it wasn't.
*/
std::string WebCache::checkCache(std::string httpRequest){
	std::map<std::string, CachedPage>::iterator it;
	m_cacheLock.lockForRead(); 
	it = m_cache.find(httpRequest);
	bool found = !(it == m_cache.end());
	m_cacheLock.unlock();
	//If the request was found in the cache
	if(found){
		//If the page is expired, erase it.
		if(isPageExpired(it->second)){
			m_cacheLock.lockForWrite(); 
			m_cache.erase(it);
			m_cacheLock.unlock();
		} else {
			return (it->second).m_page;
		}
	}
	return NULL;
}

/*Returns true if the cached page expired, based on the 
	current system time.
*/
bool WebCache::isPageExpired(CachedPage page){
	time_t currentTime;
	time(&currentTime);
	return (difftime(page.m_expireTime, currentTime) < 0);
}

WebCache::CachedPage::CachedPage(std::string page, std::string expTime){
	m_page = page;
	tm* t = new tm(); 
	strptime(expTime.c_str(), "%a, %d %b %Y %H:%M:%S",t);
	m_expireTime = mktime(t);
}