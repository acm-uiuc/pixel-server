CC=gcc
CFLAGS=-I.
DEPS = socket.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

socket: socket.o
	$(CC) -o socket socket.o
	rm socket.o

framebuffer: framebuffer.c
	$(CC) -g -Wall -o framebuffer framebuffer.c

all: socket framebuffer

clean:
	rm socket framebuffer
