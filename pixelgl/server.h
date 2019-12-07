#ifndef SERVER_H
#define SERVER_H

#define MAX_CONNECTIONS 50

#include "structs.h"
#include <cstdio>
#include <unistd.h>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <fcntl.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/epoll.h>

extern unsigned int scale;
extern unsigned int width;
extern unsigned int height;
extern float targetX;
extern float targetY;
extern std::queue<struct Request*> requestQueue;
extern std::mutex queueMutex;
extern std::mutex runningMutex;

int startNetworkThread();
int stopNetworkThread();

#endif
