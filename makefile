CC=gcc
CFLAGS= -Wextra -O2

build:
	$(CC) $(CFLAGS) -o pixel pixel.c framebuffer.c

