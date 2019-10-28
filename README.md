# Framebuffer
Backend for ACM Pixel.

It works by loading the Linux Framebuffer (located in /dev/fb0) into system memory so we can quickly edit the pixels on the screen.
It uses sockets to listen to incoming requests. 

To POST

Curl:
```
curl -X POST -d 'x=?&y=?&r=?&g=?&b=?' pixel.acm.illinois.edu (Or Local IP if running locally)
```

# Endpoints

POST
Renders a pixel with specified pixel color, at the given x and y coordinates.

Request body:
x: 0-250
y: 0-250
color: RGB format.


GET /small.bmp

Fetches a 250x250 screenshot of the Pixel display.

