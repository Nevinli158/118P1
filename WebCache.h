#ifndef WEB_CACHE_H
#define WEB_CACHE_H

#include <map>
#include <string>
#include <time.h>

#include "ReadWriteLock.h"

class WebCache {

	public:
		WebCache();
		void cachePage(std::string httpRequest, std::string page, std::string expTime);
		std::string checkCache(std::string httpRequest);
		
	private:
		class CachedPage{
			public: 
				CachedPage(std::string page, std::string expTime);
				std::string m_page;
				time_t m_expireTime;
		};
		std::map<std::string, CachedPage> m_cache;
		bool isPageExpired(CachedPage page);
		ReadWriteLock m_cacheLock;
		
};

#endif