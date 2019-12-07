#ifndef STRUCTS_H
#define STRUCTS_H

struct Pixel
{
    float startX, endX, startY, endY;
    unsigned char r, g, b;
};

struct Request
{
    unsigned int x, y;
    unsigned char r, g, b;
};

#endif