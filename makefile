CC=gcc
CFLAGS= -pthread -Wextra -O2

build:
	$(CC) $(CFLAGS) -pthread -o pixel pixel.c framebuffer.c

