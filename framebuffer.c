#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/kd.h>
#include <linux/input.h>

int fbfd = 0;
int console;
int keyboard;
struct input_event keyEvent;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
long int screensize = 0;
char *fbp = 0;
int x = 0, y = 0;
long int location = 0;

int loadFrameBuffer()
{
	fbfd = open("/dev/fb0", O_RDWR);
	if (fbfd == -1) 
	{
		perror("Error: cannot open framebuffer device\n");
		return -1;
	}
	printf("The framebuffer device was opened successfully.\n");

	// Get fixed screen information
	if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) 
	{
		perror("Error reading fixed information\n");
		return -1;
	}

	// Get variable screen information
	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
		perror("Error reading variable information\n");
		return -1;
	}
	return 1;
}

int loadConsole()
{
	console = open("/dev/tty0", O_RDWR);
	if (console == -1) 
	{
		perror("Error: cannot open framebuffer device\n");
		return -1;
	}
	return 1;
}

void enableConsoleGraphics()
{
	if (ioctl(console, KDSETMODE, KD_GRAPHICS) == -1)
	{
		printf("Error setting console\n");
	}
}

void disableConsoleGraphics()
{
	if(ioctl(console, KDSETMODE, KD_TEXT) == -1)
	{
		printf("Could not switch to text\n");
	}
}

void loadKeyBoard()
{
	keyboard = open("/dev/input/event2", O_RDONLY);
}

void closeKeyBoard()
{
	close(keyboard);
}

int main()
{
	loadKeyBoard();
	// Open the file for reading and writing
	if(loadFrameBuffer() != 1)
		return -1;

	printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

	if(loadConsole() != 1)
		return -1;

	enableConsoleGraphics();

	// Figure out the size of the screen in bytes
	screensize = finfo.smem_len;
	// Map the device to memory
	fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);

	if ((int)fbp == -1) 
	{
		perror("Error: failed to map framebuffer device to memory\n");
		exit(4);
	}
	printf("The framebuffer device was mapped to memory successfully.\n");


	for (y = 0; y < vinfo.yres; y++)
	{
		for (x = 0; x < vinfo.xres; x+=4)
		{
			int r = rand() % 256;
			int g = rand() % 256;
			int b = rand() % 256;
			for (int i = 0; i < 4; i++)
			{
				location = (x+vinfo.xoffset + i) * (vinfo.bits_per_pixel/8) +
					(y+vinfo.yoffset) * finfo.line_length;
				*(fbp + location) = r;
				*(fbp + location + 1) = g;
				*(fbp + location + 2) = b;
				*(fbp + location + 3) = 0;
			}
		}
	}

	while (1)
	{
		read(keyboard, &keyEvent, sizeof(struct input_event));
		if (keyEvent.type == 1 && keyEvent.code == 16)
		{
			munmap(fbp, screensize);
			close(fbfd);
			closeKeyBoard();
			disableConsoleGraphics();
			close(console);
			break;
		}
	}
	return 0;
}