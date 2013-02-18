#ifndef BUFFER_H
#define BUFFER_H

#define MAXBUFSIZE 1024


class Buffer {
public:
	Buffer();
	void add(Buffer *b, size_t s);
	void add(char *c, size_t s);
	void add(char c);
	void clear();
	void grow();
	void print();
	~Buffer();
	
	char *buf;
	size_t size;
	size_t maxsize;
};

#endif
