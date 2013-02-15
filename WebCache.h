#ifndef WEB_CACHE_H
#define WEB_CACHE_H

#include <map>
#include <string>
#include <time.h>

#include "ReadWriteLock.h"

class WebCache {

	public:
		WebCache();
		//Adds the page associated with the httprequest to the cache.
		void cachePage(std::string httpRequest, std::string page, std::string expTime);
		//Checks the cache for the given request, and returns the page if it's available and not timed out.
		std::string checkCache(std::string httpRequest);
		
	private:
		class CachedPage{
			public: 
				CachedPage(std::string page, std::string expTime);
				std::string m_page;
				time_t m_expireTime;
		};
		
		//Returns whether or not the cached page is expired.
		bool isPageExpired(CachedPage page);
		
		std::map<std::string, CachedPage> m_cache;
		ReadWriteLock m_cacheLock;
		
};

#endif