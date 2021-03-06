#include <cstdlib>
#include <string.h>
#include <iostream>
#include "buffer.h"

// Constructor
Buffer::Buffer() {
	size = 0;
	maxsize = MAXBUFSIZE;
	buf = (char *) malloc(maxsize * sizeof(char));
}

// Copy Constructor
Buffer::Buffer(Buffer& b) {
	size = b.size;
	maxsize = b.maxsize;
	buf = (char *) malloc(maxsize * sizeof(char));
	
	strncpy(buf, b.buf, b.size);
}

// Copy into buf s amount of characters from b
void Buffer::add(Buffer *b, size_t s) {
	while(size + s >= maxsize) {
		grow();
	}
	strncpy(buf + size, b->buf, s);
	size += s;
}

// Copy into buf s amount of characters from c
void Buffer::add(char *c, size_t s) {
	while(size + s >= maxsize) {
		grow();
	}
	strncpy(buf + size, c, s);
	size += s;
}

// Add c to the end of buf
void Buffer::add(char c) {
	if(size + 1 >= maxsize) {
		grow();
	}
	buf[size] = c;
	size++;
}

// Clear the buffer
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

// Print out the contents of buf
void Buffer::print() {
	for(unsigned int i = 0; i < size; i++) {
		std::cout << buf[i];
	}
	std::cout << std::endl;
}

Buffer::~Buffer() {
	free(buf);
	buf = NULL;
	size = maxsize = 0;
}

