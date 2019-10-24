#ifndef STRUCTS_H
#define STRUCTS_H

struct pixelData
{
	int x;
	int y;
	unsigned char r;
	unsigned char g;
	unsigned char b;
};

struct postData
{
	char name;
	unsigned char value;
};


struct request
{
	char * ip;
	struct pixelData currentData;
	int oldX;
	int oldY;
};

#endif
