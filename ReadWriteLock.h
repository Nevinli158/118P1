#ifndef RWLOCK_H
#define RWLOCK_H

#include <pthread.h>

class ReadWriteLock{

	public:
		ReadWriteLock();
		void lockForRead();
		void lockForWrite();
		void unlock();
	private:
		int readers;
		int writer;
		int pending_writers;
		pthread_cond_t readers_proceed;
		pthread_cond_t writer_proceed;
		pthread_mutex_t rw_lock;
};

#endif