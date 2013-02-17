#include <cstdlib>
#include <string.h>
#include "buffer.h"

Buffer::Buffer() {
	buf = (char *) malloc(maxsize * sizeof(char));
	size = 0;
	maxsize = MAXBUFSIZE;
}

// Copy into buf s amount of characters from b
void Buffer::add(Buffer *b, size_t s) {
	if(size + s >= maxsize) {
		grow();
	}
	strncpy(buf + size, b->buf, s);
	size += s;
}

void Buffer::clear() {
	if(size > 0) {
		for(unsigned int i = 0; i < maxsize; i++) {
			buf[i] = '\0';
		}
		size = 0;
	}
}

// Double the buffer size
void Buffer::grow() {
	maxsize *= 2;
	buf = (char *)realloc(buf, maxsize * sizeof(char));
}

Buffer::~Buffer() {
	free(buf);
	buf = NULL;
	size = maxsize = 0;
}
