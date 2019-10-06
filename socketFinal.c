#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "socket.h"
int8_t processRequest(uint8_t buffer[]) 
{
	printf("Processing Request\n");
	uint8_t header[4];
	strncpy(header, buffer, 4);
	if (strcmp(header, "POST") != 0)
	{
		printf("ERROR: This server only processes POST requests\n");
		return -1;
	}
	//TODO: Finish splitting request at new line
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
	return 0;
}

int main() 
{
	uint32_t requestCount = 0;
	int32_t listenfd, connfd, n;
	struct sockaddr_in serverAdress;
	uint8_t buff[4096 + 1];
	uint8_t recvLine[4096 + 1];

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{
		printf("ERROR: Failed to create socket\n");
		return -1;
	}

	bzero(&serverAdress, sizeof(serverAdress));
	serverAdress.sin_family = AF_INET;
	serverAdress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAdress.sin_port = htons(1800);

	if ((bind(listenfd, (struct sockaddr *) &serverAdress, sizeof(serverAdress))) < 0)
	{
		printf("ERROR: Failed to bind to the server\n");
		return -1;
	}

	if ((listen(listenfd, 10)) < 0)
	{
		printf("ERROR: Failed to listen for connections\n");
		return -1;
	}

	printf("Starting server\n");
	
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
		requestCount++;
		if (result == -27)
			break;
		printf("Processed Request: %d\n", requestCount);
		uint8_t response[] = "The server recieved your request!\n";
		write(connfd, &response, sizeof(response));
		close(connfd);
	}
	printf("Exiting server\n");
	close(listenfd);
}