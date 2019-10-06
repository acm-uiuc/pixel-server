#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/kd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <linux/input.h>

int fb = 0;
int console;
int keyboard;
struct input_event keyEvent;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
long int screensize = 0;
unsigned char *frameBuffer = 0;
int x = 0, y = 0;
long int location = 0;
double yScale = 1024.0;
double xScale = 1280.0;
double targetXRes;
double targetYRes;

//socket variables

struct sockaddr_in serverAddress;
int socket, client, readCount;
unsigned char data[4096 + 1];

// open socket

int openSocket()
{
}

int loadFrameBuffer()
{
	fb = open("/dev/fb0", O_RDWR);
	if (fb == -1) 
	{
		printf("Error: cannot open framebuffer device\n");
		return -1;
	}
	printf("The framebuffer device was opened successfully.\n");

	// Get fixed screen information
	if (ioctl(fb, FBIOGET_FSCREENINFO, &finfo) == -1) 
	{
		printf("Error reading fixed information\n");
		return -1;
	}

	// Get variable screen information
	if (ioctl(fb, FBIOGET_VSCREENINFO, &vinfo) == -1) {
		printf("Error reading variable information\n");
		return -1;
	}
	return 1;
}

int loadConsole()
{
	console = open("/dev/tty0", O_RDWR);
	if (console == -1) 
	{
		printf("Error: cannot open framebuffer device\n");
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

void loadScale(double xRes, double yRes)
{
	printf("Loading Scale (%f, %f): ", xRes, yRes);
	if (xRes < 1.0 || yRes < 1) 
	{
		printf("ERROR:Use proper scaling!\n");
	} 
	else 
	{
		targetXRes = xRes;
		targetYRes = yRes;
		xScale = (double) (vinfo.xres / xRes);
		yScale = (double) (vinfo.yres / yRes);
		printf("xRes = %f, yRes = %f, xScale = %f, yScale = %f\n");
	}
}


void drawPixel(int xPos, int yPos, int r, int g, int b, int a)
{
	printf("Draw Request: (%d, %d) -> ", xPos, yPos);
	if ((xPos < 0 || xPos >= targetXRes) || (yPos < 0 || yPos >= targetYRes)) 
	{
		printf("ERROR: Pixel position out of bounds\n");
		return;
	}
	xPos *= xScale;
	yPos *= yScale;
	printf("Drawing pixel at: (%d, %d). RGB = (%d, %d, %d)\n", xPos, yPos, r, g, b);
	for (int i = 0; i <= xScale; i++)
	{
		for (int j = 0; j <= yScale; j++)
		{
			location = (xPos + vinfo.xoffset + i) * (vinfo.bits_per_pixel/8) + (yPos + vinfo.yoffset + j) * finfo.line_length;
			*(frameBuffer + location) = b;
			*(frameBuffer + location + 1) = g;
			*(frameBuffer + location + 2) = r;
			*(frameBuffer + location + 3) = a;

		}
	}
}

int main()
{
	//Redirect printf to log.txt - Overwrites last log file
	freopen("log.txt", "w", stdout);
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
	frameBuffer = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);

	if ((int)frameBuffer == -1) 
	{
		printf("Error: failed to map framebuffer device to memory\n");
		return -1;
	}
	printf("The framebuffer device was mapped to memory successfully.\n");

	loadScale(32.0, 32.0);

	srand(time(0));

	for (y = 0; y < targetYRes; y++)
	{
		for (x = 0; x < targetXRes; x++)
		{
			int r = rand() % 256;
			int g = rand() % 256;
			int b = rand() % 256;
			drawPixel(x, y, r, g, b, 0);
		}
	}
	
	drawPixel(64, 64, 0, 0, 0, 0);

	while (1)
	{
		read(keyboard, &keyEvent, sizeof(struct input_event));
		if (keyEvent.type == 1 && keyEvent.code == 16)
		{
			munmap(frameBuffer, screensize);
			close(fb);
			closeKeyBoard();
			disableConsoleGraphics();
			close(console);
			system("clear");
			break;
		}
	}
	return 0;
}
