#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

int main() 
{
	int listenfd, connfd, n;
	struct sockaddr_in serverAdress;
	uint8_t buff[4096 + 1];
	uint8_t recvLine[4096 + 1];

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{
		printf("Error\n");
		return -1;
	}

	bzero(&serverAdress, sizeof(serverAdress));
	serverAdress.sin_family = AF_INET;
	serverAdress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAdress.sin_port = htons(1800);
	
	if ((bind(listenfd, (struct sockaddr *) &serverAdress, sizeof(serverAdress))) < 0)
	{
		printf("Error 2\n");
		return -1;
	}

	if ((listen(listenfd, 10)) < 0)
	{
		printf("Error 3\n");
		return -1;
	}

	while (1)
	{
		struct sockaddr_in addr;
		socklen_t addr_len;

		printf("Waiting for connection\n");
		fflush(stdout);
		
		connfd = accept(listenfd, (struct sockaddr *) NULL, NULL);

		memset(recvLine, 0, 4096);

		while ((n = read(connfd, recvLine, 4096 - 1)) > 0)
		{
			printf("%s\n", recvLine);

			if (recvLine[n - 1] == '\n')
				break;

			memset(recvLine, 0, 4096);
		}

		if (n < 0) 
		{
			printf("Read negative bytes\n");
			return -1;
		}
	//	write(connfd, (char*) buff, strlen((char *)buff));
		close(connfd);
	}
}
