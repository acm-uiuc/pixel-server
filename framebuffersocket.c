#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/kd.h>
#include <linux/input.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

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

//TODO: Abstract Socket code into socket.h
//Socket Variables

int listenfd, connfd, n;
struct sockaddr_in serverAdress;
uint8_t buff[4096 + 1];
uint8_t recvLine[4096 + 1];

//Socket Methods

int openSocket()
{
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{
		printf("Error\n");
		return -1;
	}

	bzero(&serverAdress, sizeof(serverAdress));
	serverAdress.sin_family = AF_INET;
	serverAdress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAdress.sin_port = htons(1800);
	return 0;
}

int bindSocket()
{
	if ((bind(listenfd, (struct sockaddr *) &serverAdress, sizeof(serverAdress))) < 0)
	{
		printf("Error 2\n");
		return -1;
	}
	return 0;
}

//Processing request

int8_t processRequest(uint8_t buffer[]) 
{
	printf("Processing Request\n");
	uint8_t header[4];
	//TODO: Fix request identification
	/*
	   strncpy(header, buffer, 4);
	   if (strcmp(header, "POST") != 0)
	   {
	   printf("ERROR: This server only processes POST requests\n");
	   return -1;
	   }
	 */
	uint8_t **array = (uint8_t**) malloc(24);
	uint8_t * token;
	token = strtok(buffer, "\n");
	int32_t index = 0;
	while (token != NULL) 
	{
		*(array + index) = token;
		index += 1;
		token = strtok(NULL, "\n");
	}
	uint8_t * request = *(array + index - 1);
	printf("%s\n", request);
	free(array);
	if (strncmp(request, "action=exit", 11) == 0)
	{
		return -27;
	}
	uint8_t **params = (uint8_t**) malloc(16);
	uint8_t *paramToken = strtok(request, ",");
	int paramCount = 0;
	while (paramToken != NULL)
	{
		*(params + paramCount) = paramToken;
		paramCount++;
		paramToken = strtok(NULL, ",");
	}

	uint8_t data[5];
	int equalityIndex = 0;
	for (int i = 0; i < paramCount; i++)
	{
		if (*(params + i) == NULL)
			break;
		//process each value
		int j = 0;
		printf("Parsing %s, with length = %d\n", *(params + i), strlen(*(params + i)));

		for (j = 0; j < strlen(*(params + i)); j++)	
		{
			if ((u_int8_t)*(*(params + i) + j) == '=')
			{
				equalityIndex = j + 1;
				break;
			}
		}
		data[i] = atoi(&(*(*(params + i) + equalityIndex)));		
		equalityIndex = 0;
		printf("Data at %d = %d\n", i, data[i]);
	}

	drawPixel(data[0], data[1], data[2], data[3], data[4], 0);

	free(params);
	return 0;
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
	printf("Loading Scale (%f, %f): \n", xRes, yRes);
	if (xRes < 1.0 || yRes < 1 || xRes > vinfo.xres || yRes > vinfo.yres) 
	{
		printf("ERROR:Use proper scaling!\n");
	} 
	else 
	{
		targetXRes = xRes;
		targetYRes = yRes;
		xScale = (double) (vinfo.xres / xRes);
		yScale = (double) (vinfo.yres / yRes);
		printf("xRes = %f, yRes = %f, xScale = %f, yScale = %f\n", targetXRes, targetYRes, xScale, yScale);
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
	for (int i = 0; i < xScale; i++)
	{
		for (int j = 0; j < yScale; j++)
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
	printf("Starting framebuffer...\n");
	//Redirect printf to log.txt - Overwrites last log file
	//	freopen("log.txt", "w", stdout);
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
	frameBuffer = (unsigned char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);

	if ((int)frameBuffer == -1) 
	{
		printf("Error: failed to map framebuffer device to memory\n");
		return -1;
	}
	printf("The framebuffer device was mapped to memory successfully.\n");

	loadScale(64.0, 64.0);

	srand(time(0));
	printf("Starting to clear screen...\n");
	for (y = 0; y < targetYRes; y++)
	{
		for (x = 0; x < targetXRes; x++)
		{
			int r = rand() % 256;
			int g = rand() % 256;
			int b = rand() % 256;
			drawPixel(x, y, 255, 0, 0, 0);
		}
	}
	printf("Cleared Screen\n");
	
	//OLD FRAMEBUFFERCODE
	// while (1)
	// {
	// 	read(keyboard, &keyEvent, sizeof(struct input_event));
	// 	if (keyEvent.type == 1 && keyEvent.code == 16)
	// 	{
	// 		munmap(frameBuffer, screensize);
	// 		close(fb);
	// 		closeKeyBoard();
	// 		disableConsoleGraphics();
	// 		close(console);
	// 		system("clear");
	// 		break;
	// 	}
	// }

	printf("Opening Socket= %d\n", openSocket());
	printf("Binding Socket = %d\n", bindSocket());

	if ((listen(listenfd, 10)) < 0)
	{
		printf("ERROR: Failed to listen for connections\n");
		return -1;
	}

	while (1)
	{
		struct sockaddr_in addr;
		socklen_t addr_len;

		printf("\nWaiting for connection\n");

		connfd = accept(listenfd, (struct sockaddr *) NULL, NULL);

		memset(recvLine, 0, 4096);

		if ((n = read(connfd, recvLine, 4096 - 1)) > 0)
			memcpy(buff, recvLine, sizeof(buff));
		if (n < 0) 
		{
			printf("ERROR: Read negative bytes\n");
			return -1;
		}
		int8_t result = processRequest(recvLine);
		if (result == -27)
			break;
		//TODO: Edit the response so it reflects the statust of the request (i.e: Error codes or if the request was processed)
		uint8_t response[256] = "The server recieved your request!\n";
		write(connfd, (char *) &response, sizeof(response));
		close(connfd);
	}
	//TODO: Abstract the following lines into a cleanup function
	printf("Exiting server\n");
	close(listenfd);
	munmap(frameBuffer, screensize);
	close(fb);
	closeKeyBoard();
	disableConsoleGraphics();
	close(console);
	system("clear");


	return 0;
}
