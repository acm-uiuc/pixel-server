CC=gcc
CFLAGS= -Wextra -O2
DFLAGS = -Wextra -O2 -ggdb3
LFLAGS = -pthread -lpng -ljpeg
build:
	$(CC) $(CFLAGS) -o pixel pixel.c framebuffer.c $(LFLAGS)

debug:
	$(CC) $(CFLAGS) -o pixel pixel.c framebuffer.c $(LFLAGS)
