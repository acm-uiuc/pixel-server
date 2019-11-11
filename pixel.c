#define MAX_CONNECTIONS 50
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
#include <netdb.h>
#include "structs.h"

int listenfd, connfd, n;
struct sockaddr_in serverAdress;
struct addrinfo hints;
struct addrinfo *serverInfo;
char recvLine[4096 + 1];
char * response;

/**
 * Opens the socket for the server to accept incoming requests.
 * @return: 1 if the socket opened properly, -1 if it failed.
 */

int openSocket()
{
	// if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	// {
	// 	printf("Error opening socket!\n");
	// 	return -1;
	// }

	// bzero(&serverAdress, sizeof(serverAdress));
	//serverAdress.sin_family = AF_INET;
	//serverAdress.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if(getaddrinfo(NULL, "1800", &hints, &serverInfo) != 0)
		return -1;
	listenfd = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
	//serverAdress.sin_addr.s_addr = inet_addr("192.17.239.50");
	//serverAdress.sin_port = htons(1800);
	return 0;
}

/**
 * Binds the socket that was opened.
 * @return: 1 if successful, -1 if binding failed.
 */

int bindSocket()
{
	// if ((bind(listenfd, (struct sockaddr *) &serverAdress, sizeof(serverAdress))) < 0)
	// {
	// 	printf("Error binding socket!\n");
	// 	return -1;
	// }
	if (bind(listenfd, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1)
		return -1;
	return 0;
}

/**
 * Processes the current request given to the server.
 * @param buffer: The string representation of the request.
 * @return: 0 if the request was processed successfully.
 * @return: 1 if the request is trying to fetch the current state of pixel.
 * @return: -2 if the request has too many parameters.
 * @return: -3 if the request values were out of bounds.
 * @return: -4 if the request did not recieve enough information
 * @return: -27 exits the server. WARNING: Deprecated return value in the production build.
 */

char processRequest(char* buffer) 
{
	printf("Processing Request\n");

	char header[14];

	strncpy(header, buffer, 14);
	if (strncmp(header, "GET /state.jpg", 14) == 0)
	{
		return 1;
	}

	char * request;
	char query[8];
	strncpy(query, buffer, 7);
	if (strncmp(query, "POST /?", 7) == 0) {
		strtok(buffer, "?");
		buffer = strtok(NULL, "?");
		request = strtok(buffer, "HTTP");
	}
	else
	{
		char *array[16];
		char * token;
		token = strtok(buffer, "\n");
		int index = 0;
		while (token != NULL) 
		{
			printf("Line: %d = %s\n", index, token);
			array[index] = token;
			index += 1;
			token = strtok(NULL, "\n");
		}

		request = array[index - 1];
	}
	//if (strncmp(request, " ", 1) == 0)
	//	return -3;
	printf("%s\n", request);

	if (strncmp(request, "action=exit", 11) == 0)
	{
		return -27;
	}
	
	unsigned char *params[16];
	unsigned char *paramToken = strtok(request, "&");
	int paramCount = 0;
	while (paramToken != NULL)
	{
		params[paramCount] = paramToken;
		paramCount++;
		paramToken = strtok(NULL, "&");
	}

	if (paramCount > 5)
		return -2;
	
	if (paramCount < 5)
		return -4;
	
        struct postData parsedParameters[paramCount];
	struct pixelData pxl = {.x = 0, .y= 0, .r=0, .g = 0, .b = 0};
	int equalityIndex = 0;
	for (int i = 0; i < paramCount; i++)
	{
		if (*(params + i) == NULL)
			break;
		//process each value
		int j = 0;
		printf("Parsing %s, with length = %d\n", params[i]), strlen(params[i]);

		for (j = 0; j < strlen(params[i]); j++)	
		{
			if ((unsigned char)*(params[i] + j) == '=')
			{
				equalityIndex = j + 1;
				break;
			}
		}
		(parsedParameters+i)->value = atoi(&(*(params[i] + equalityIndex)));
		(parsedParameters+i)->name = (char) *(params[i]);
		equalityIndex = 0;
		printf("Data at %d = %d\n", i, parsedParameters[i].value);
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
		if ( pxl.x >= 0 && pxl.y >= 0 && pxl.r >= 0 && pxl.g >= 0 && pxl.b >= 0 && pxl.r <256 && pxl.g < 256 && pxl.b <256 && pxl.x < targetXRes && pxl.y < targetYRes) {
			drawPixel(pxl.x, pxl.y, pxl.r, pxl.g, pxl.b, 0);
			return 0;
		}
		return -3;
	}
	
	return -4;
}

/**
 * Sends the current state of pixel to the client.
 * @param client: Pointer to the client file descriptor.
 * @return: -1 for failure, 0 for success.
 */

int sendImage(int* client)
{
	FILE* output;
	int size;
	output = fopen("state.jpg", "rb");
	if (output == NULL)
	{
		printf("Error reading from image file!\n");
		return -1;
	}
	char imageBuffer[1024];
	fseek(output, 0, SEEK_END);
	size = ftell(output);
	fseek(output, 0, SEEK_SET);
	sprintf(response, "HTTP/1.1 200 OK\nContent-Type: image/jpeg\nContent-Length: %d\n\n", size);
	write(*client, response, strlen(response));
	while (!feof(output))
	{
		fread(&imageBuffer, sizeof(imageBuffer), 1, output);
		write(*client, &imageBuffer, sizeof(imageBuffer));
	}
	if (fclose(output) != 0)
	{
		printf("Error closing image file!\n");
		return -1;
	}
	return 0;
}

int main()
{
	int openResult = openSocket();
	int bindResult = bindSocket();
	if (openResult < 0 || bindResult < 0)
		return -1;

	printf("Starting framebuffer\n");
	loadFrameBuffer();
	loadScale(128, 128);
	clearScreen(255, 0, 0);
	saveState();

	if ((listen(listenfd, MAX_CONNECTIONS)) < 0)
	{
		printf("ERROR: Failed to listen for connections\n");
		return -1;
	}
	response = (unsigned char*) malloc(256);
	while (running)
	{
		struct sockaddr_in client;
		socklen_t clientLen;

		printf("\nWaiting for connection\n");

		connfd = accept(listenfd, (struct sockaddr*) &client, &clientLen);

		printf("Recieved connection from %s\n", inet_ntoa(client.sin_addr));	

		//memset(recvLine, 0, 4096);
		bzero(recvLine, 4096);
		if ((n = read(connfd, recvLine, 4096 - 1)) > 0)
			//	strncpy(buff, recvLine, sizeof(rec));
			//memcpy(buff, recvLine, sizeof(buff));
			if (n < 0) 
			{
				printf("ERROR: Read negative bytes\n");
			}
		printf("\nPrinting request: %s\n", recvLine);
		char result = processRequest(recvLine);
		printf("Process result = %d\n", result);
		if (result == -27)
		{
			running = false;
			strcpy(response, "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 15\n\nExiting server\n");
			write(connfd, response, strlen(response));
		}
		if (result == 0)
		{
			strcpy(response, "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 34\n\nThe server recieved your request!\n");
			write(connfd, response, strlen(response));
		}

		if (result == -1)
		{
			strcpy(response, "HTTP/1.1 400 BAD REQUEST\nContent-Type: text/plain\nContent-Length: 61\n\nNot enough information was recieved in order to draw a pixel\n");
			write(connfd, response, strlen(response));
		}
		if (result == -2)
		{
			strcpy(response, "HTTP/1.1 400 BAD REQUEST\nContent-Type: text/plain\nContent-Length: 71\n\nSome fields were sent multiple times, so your request wasn't processed\n");
			write(connfd, response, strlen(response));
		}
		if (result == -3)
		{
			strcpy(response, "HTTP/1.1 400 BAD REQUEST\nContent-Type: text/plain\nContent-Length: 31\n\nSome values were out of bounds\n");
			write(connfd, response, strlen(response));
		}
		if (result == -4)
		{
			strcpy(response, "HTTP/1.1 400 BAD REQUEST\nContent-Type: text/plain\nContent-Length: 31\n\nSome values were out of bounds\n");
			write(connfd, response, strlen(response));
		}
		if (result == 1)
		{
			printf("Recieved Send image signal\n");
			printf("Send output: %d\n", sendImage(&connfd));
		}
		//write(connfd, response, strlen(response));
		close(connfd);
		//memset(response, 0, strlen(response));
		bzero(response, strlen(response));
	}
	close(connfd);
	close(listenfd);
	printf("Exiting server...\n");
	printf("Closing framebuffer...\n");
	free(response);
	printf("Freeing HTTP\n");
	closeFrameBuffer();
	printf("Final Instruction\n");
	return 0;
}

