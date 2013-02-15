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
	memcpy(buf + size, b->buf, s);
	size += s;
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

