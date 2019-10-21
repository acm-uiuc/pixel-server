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
	unsigned char **array = (unsigned char**) malloc(24);
	unsigned char * token;
	token = strtok(buffer, "\n");
	int index = 0;
	while (token != NULL) 
	{
		*(array + index) = token;
		index += 1;
		token = strtok(NULL, "\n");
	}
	unsigned char * request = *(array + index - 1);
	printf("%s\n", request);
	free(array);
	if (strncmp(request, "action=exit", 11) == 0)
	{
		return -27;
	}
	unsigned char **params = (unsigned char**) malloc(16);
	unsigned char *paramToken = strtok(request, ",");
	int paramCount = 0;
	while (paramToken != NULL)
	{
		*(params + paramCount) = paramToken;
		paramCount++;
		paramToken = strtok(NULL, ",");
	}

	unsigned char data[5];
	int equalityIndex = 0;
	for (int i = 0; i < paramCount; i++)
	{
		if (*(params + i) == NULL)
			break;
		//process each value
		int j = 0;
		//printf("Parsing %s, with length = %d\n", *(params + i), (int) strlen(*(params + i)));

		for (j = 0; j < (int) strlen(*(params + i)); j++)	
		{
			if ((unsigned char)*(*(params + i) + j) == '=')
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
	drawCount++;
	free(params);
	return 0;
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
		//TODO: Edit the response so it reflects the statust of the request (i.e: Error codes or if the request was processed)
		strcpy(response, "The server recieved your request!\n");
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

