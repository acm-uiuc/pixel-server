#include "framebuffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <linux/input.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

struct pixelData
{
	int x, y, r, g, b;
};

struct postData
{
	char name;
	int value;
};

int listenfd, connfd, n;
struct sockaddr_in serverAdress;
unsigned char buff[4096 + 1];
unsigned char recvLine[4096 + 1];
unsigned char * response;
unsigned int drawCount = 0;

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

char processRequest(unsigned char buffer[]) 
{
	printf("Processing Request\n");
	/*
	unsigned char *header = malloc((unsigned char *) 4);
	//TODO: Fix request identification
	
	strncpy(header, buffer, 4);
	if (strcmp(header, "POST") != 0)
	{
		printf("ERROR: This server only processes POST requests\n");
		return -1;
	}
	
	free(header);
	*/
	unsigned char *array[8];
	unsigned char * token;
	token = strtok(buffer, "\n");
	int index = 0;
	while (token != NULL) 
	{
		array[index] = token;
		index += 1;
		token = strtok(NULL, "\n");
	}
	unsigned char * request = array[index - 1];
	printf("%s\n", request);
	if (strncmp(request, "action=exit", 11) == 0)
	{
		return -27;
	}
	unsigned char *params[10];
	unsigned char *paramToken = strtok(request, "&");
	int paramCount = 0;
	while (paramToken != NULL)
	{
		params[paramCount] = paramToken;
		paramCount++;
		paramToken = strtok(NULL, "&");
	}

	struct postData parsedParameters[paramCount];
	struct pixelData pxl;
	int equalityIndex = 0;
	int tempValue = 0;
	for (int i = 0; i < paramCount; i++)
	{
		if (params[i] == NULL)
			break;
		//process each value
		int j = 0;
		printf("Parsing %s, with length = %d\n", params[i], strlen(params[i]));
		for (j = 0; j < strlen(params[i]); j++)	
		{
			if (*(params[i] + j) == '=')
			{
				equalityIndex = j;
				break;
			}
		}
		tempValue = atoi((params[i] + equalityIndex + 1));
		printf("found value %d\n", tempValue);
		(parsedParameters+i)->name = (char) *(params[i]);
		(parsedParameters+i)->value = tempValue;
		equalityIndex = 0;
	}

	bool r = false;
	bool g = false;
	bool b = false;
	bool x = false;
	bool y = false;

	for (int i = 0; i < paramCount; i++) {
		printf("1\n");
		if (parsedParameters[i].name == "r"[0]) {
			if (r) {
				return -2;
			}
			pxl.r = parsedParameters[i].value;
			r = true;
			printf("2\n");
		}
		if (parsedParameters[i].name == "g"[0]) {
			if (g) {
				return -2;
			}
			pxl.g = parsedParameters[i].value;
			g = true;
			printf("2\n");
		}
		if (parsedParameters[i].name == "b"[0]) {
			if (b) {
				return -2;
			}
			pxl.b = parsedParameters[i].value;
			b = true;
			printf("2\n");
		}
		if (parsedParameters[i].name == "x"[0]) {
			if (x) {
				return -2;
			}
			pxl.x = parsedParameters[i].value;
			x = true;
			printf("2\n");
		}
		if (parsedParameters[i].name == "y"[0]) {
			if (y) {
				return -2;
			}
			pxl.y = parsedParameters[i].value;
			y = true;
			printf("2\n");
		}
	}

	if (x && y && r && g && b) {
		if ( pxl.x >= 0 && pxl.y >= 0 && pxl.r >= 0 && pxl.g >= 0 && pxl.b >= 0 && pxl.x < targetXRes && pxl.y < targetYRes && pxl.r < 256 && pxl.g < 256 && pxl.b < 256) {
			drawPixel(pxl.x,pxl.y,pxl.r,pxl.g,pxl.b,0);
			drawCount++;
			return 0;
		} else {
			return -3;
		}
	}

	return -1;
}

int main()
{
	int openResult = openSocket();
	int bindResult = bindSocket();
	if (openResult < 0 || bindResult < 0)
	{
		printf("Error opening and binding socket\n");
		return -1;
	}

	printf("Starting framebuffer\n");
	loadFrameBuffer();
	loadScale(128, 128);
	clearScreen(255, 0, 0);
	saveState();
	if ((listen(listenfd, 10)) < 0)
	{
		printf("ERROR: Failed to listen for connections\n");
		return -1;
	}
	response = (unsigned char*) malloc(256);
	while (1)
	{
		if (drawCount >= 64)
		{
	//		saveState();
			drawCount = 0;
		}

		struct sockaddr_in client;
		socklen_t clientLen;

		printf("\nWaiting for connection\n");
		
		connfd = accept(listenfd, (struct sockaddr*) &client, &clientLen);

		printf("Recieved connection from %s\n", inet_ntoa(client.sin_addr));	
		
		memset(recvLine, 0, 4096);

		if ((n = read(connfd, recvLine, 4096 - 1)) > 0)
			memcpy(buff, recvLine, sizeof(buff));
		if (n < 0) 
		{
			printf("ERROR: Read negative bytes\n");
		}
		char result = processRequest(recvLine);
		if (result == -27)
			break;
		if (result == 0)
			strcpy(response, "The server recieved your request!\n");
		
		if (result == -1)
			strcpy(response, "Not enough information was recieved in order to draw a pixel\n");

		if (result == -2)
			strcpy(response, "Some fields were sent multiple times, so your request wasn't processed\n");

		if (result == -3)
			strcpy(response, "Some values were out of bounds\n");

		write(connfd, response, strlen(response));
		close(connfd);
		memset(response, 0, strlen(response));
	}
	printf("Exiting server...\n");
	close(listenfd);
	printf("Closing framebuffer...\n");
	free(response);
	closeFrameBuffer();
}

