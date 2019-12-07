#include "server.h"

int listenfd, connfd, n;
struct sockaddr_in serverAdress;
struct addrinfo hints;
struct addrinfo *serverInfo;
char recvLine[4096 + 1];
char * response;
std::thread networkThread;
std::mutex mutex;
std::atomic_bool running;

int init()
{
    memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if(getaddrinfo(NULL, "1800", &hints, &serverInfo) != 0)
    {
        printf("Failed to generate socket details\n");
        return -1;
    }
		
	listenfd = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

    if (bind(listenfd, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1)
    {
        printf("Failed to bind socket\n");
        return -1;
    }

    return 0;
}

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
	
	char *params[16];
	char *paramToken = strtok(request, "&");
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
	
    unsigned char data[5];
	int equalityIndex = 0;
	for (int i = 0; i < paramCount; i++)
	{
		if (*(params + i) == NULL)
			break;
		//process each value
		int j = 0;
		printf("Parsing %s, with length = %u\n", params[i], (unsigned int) strlen(params[i]));

		for (j = 0; j < strlen(params[i]); j++)	
		{
			if ((unsigned char)*(params[i] + j) == '=')
			{
				equalityIndex = j + 1;
				break;
			}
		}
		
		data[i] = atoi(&(*(params[i] + equalityIndex)));
		equalityIndex = 0;
		printf("Data at %d = %d\n", i, data[i]);
	}
    if (data[0] >= scale || data[1] >= scale)
        return -3;
    struct Request* r = new struct Request;
    r->x = data[0];
    r->y = data[1];
    r->r = data[2];
    r->g = data[3];
    r->b = data[4];
    queueMutex.lock();
    requestQueue.push(r);
    queueMutex.unlock();
    printf("Pushing queue\n");
	return 0;
}

void threadFunc()
{
    printf("Starting network thread\n");
	int result = init();
    printf("Init result = %d\n", result);
	if (result == -1)
		return;
    if ((listen(listenfd, MAX_CONNECTIONS)) < 0)
	{
		printf("ERROR: Failed to listen for connections\n");
		return;
	}
    printf("Finished network init\n");
    response = new char[256];
	mutex.lock();
	bool condition = running;
	mutex.unlock();
	while (condition)
	{
		struct sockaddr_in client;
		socklen_t clientLen;

		printf("\nWaiting for connection\n");

		connfd = accept(listenfd, (struct sockaddr*) &client, &clientLen);
		if (connfd < 0)
		{
			printf("Skipping\n");
			continue;
		}
		printf("Recieved connection from %s\n", inet_ntoa(client.sin_addr));	

		memset(recvLine, 0, 4096);
		
		if ((n = read(connfd, recvLine, 4096 - 1)) > 0)
			if (n < 0) 
			{
				printf("ERROR: Read negative bytes\n");
			}
		printf("\nPrinting request: %s\n", recvLine);
		char result = processRequest(recvLine);
		printf("Process result = %d\n", result);
		if (result == -27)
		{
			strcpy(response, "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 15\n\nExiting server\n");
			write(connfd, response, strlen(response));
            break;
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
		memset(response, 0, strlen(response));
	}
	printf("Stopping network thread\n");
	close(connfd);
	close(listenfd);
	printf("Exiting server...\n");
	printf("Closing framebuffer...\n");
	delete[] response;
	printf("Freeing HTTP\n");
	printf("Final Instruction\n");
	return;
}

int startNetworkThread()
{
    printf("Creating netowrk thread\n");
	mutex.lock();
	running = true;
	mutex.unlock();
    networkThread = std::thread(threadFunc);
}

int stopNetworkThread()
{
	mutex.lock();
	running = false;
	mutex.unlock();
	printf("Joining network thread\n");
	networkThread.join();
}
