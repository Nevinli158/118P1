#ifndef WEB_CACHE_H
#define WEB_CACHE_H

#include <map>
#include <string>
#include <time.h>

#include "ReadWriteLock.h"
#include "http-request.h"
#include "buffer.h"

class WebCache {

	public:
		WebCache();
		//Adds the page associated with the httprequest to the cache.
		void cachePage(const HttpRequest* httpRequest, Buffer* page, const std::string expTime = "");
		//Checks the cache for the given request, and returns the page if it's available and not timed out.
		Buffer* checkCache(const HttpRequest* httpRequest);
		
	private:
		class CachedPage{
			public: 
				CachedPage(Buffer* page, const std::string expTime);
				Buffer* m_page;
				time_t m_expireTime;
		};
		
		//Returns whether or not the cached page is expired.
		bool isPageExpired(const CachedPage page);
		std::string parseRequestKey(const HttpRequest* httpRequest);
		
		std::map<std::string, CachedPage> m_cache;
		ReadWriteLock m_cacheLock;
};

#endif