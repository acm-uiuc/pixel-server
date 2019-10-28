CC=gcc
CFLAGS= -Wextra -O2
LFLAGS = -pthread -lpng -ljpeg
build:
	$(CC) $(CFLAGS) -o pixel pixel.c framebuffer.c $(LFLAGS)

