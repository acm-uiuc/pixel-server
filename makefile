CC=gcc
CFLAGS= -Wextra -O2

build:
	$(CC) $(CFLAGS) -pthread -o pixel pixel.c framebuffer.c

