# Framebuffer
Backend for ACM Pixel.

It works by loading the Linux Framebuffer (located in /dev/fb0) into system memory so we can quickly edit the pixels on the screen.
It uses sockets to listen to incoming requests. 
