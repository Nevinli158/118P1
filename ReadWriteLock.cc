#include <pthread.h>
#include "ReadWriteLock.h"
ReadWriteLock::ReadWriteLock(){
	readers = 0;
	writer = 0;
	pending_writers = 0;
	pthread_cond_init(&readers_proceed, NULL);
	pthread_cond_init(&writer_proceed, NULL);
	pthread_mutex_init(&rw_lock, NULL);

}

/*
	Locks the lock for reading. Multiple readers can lock
	the lock at the same time.
*/
void ReadWriteLock::lockForRead(){
	pthread_mutex_lock(&rw_lock);
	//Wait until there are no more writers/pending writers.
	while((pending_writers > 0) || (writer > 0)){
		pthread_cond_wait(&readers_proceed, &rw_lock);
	}
	//Only readers have access to the obj after this point.
	readers++;
	pthread_mutex_unlock(&rw_lock);
}

/*
	Locks the lock for writing. Only one writer can lock
	the lock at the same time.
*/
void ReadWriteLock::lockForWrite(){
	pthread_mutex_lock(&rw_lock);
	pending_writers++;
	//Wait until there are no more writers/readers.
	while( (writer > 0) || (readers > 0)){
		pthread_cond_wait(&writer_proceed,&rw_lock);
	}
	//Only this writer should have access to the obj after this point.
	pending_writers--;
	writer++;
	pthread_mutex_unlock(&rw_lock);
}

/*
	Unlocks the lock.
*/
void ReadWriteLock::unlock(){
	pthread_mutex_lock(&rw_lock);
	if(writer > 0){//unlock the write lock
		writer = 0;
	} else if(readers > 0){//unlock a read lock.
		readers--;
	}
	pthread_mutex_unlock(&rw_lock);
	if(readers == 0 && pending_writers > 0){
		//If there aren't any more readers, let a pending writer through.
		pthread_cond_signal(&writer_proceed);
	} else if(readers > 0){
		//If there aren't any pending writers, let readers through.
		pthread_cond_broadcast(&readers_proceed);
	}
}