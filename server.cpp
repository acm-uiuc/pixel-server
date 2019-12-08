#include "mongoose.h"
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>

static const char *s_http_port = "8000";

struct strPixel {
    char x[4];
    char y[4];
    char r[4];
    char g[4];
    char b[4];
};

struct pixel {
    int x,y,r,g,b;
};

char board[128 * 128 * 6+1];


int goodBounds(int a, int max) {
    if (a >= 0 && a < max)
        return 1;
    return 0;
}

int parsePixelRequest(struct http_message *hm, struct pixel *pxl) {
    struct strPixel possiblePix;
   
    if (mg_get_http_var(&(hm->body), "x", possiblePix.x, 4) <= 0 && mg_get_http_var(&(hm->query_string), "x", possiblePix.x, 4) <= 0)
        return 0;
    //printf("x: %s\n", possiblePix.x);
    if (mg_get_http_var(&(hm->body), "y", possiblePix.y, 4) <= 0 && mg_get_http_var(&(hm->query_string), "y", possiblePix.y, 4) <= 0)
        return 0;
    //printf("y: %s\n", possiblePix.y);
    if (mg_get_http_var(&(hm->body), "r", possiblePix.r, 4) <= 0 && mg_get_http_var(&(hm->query_string), "r", possiblePix.r, 4) <= 0)
        return 0;
    //printf("r: %s\n", possiblePix.r);
    if (mg_get_http_var(&(hm->body), "g", possiblePix.g, 4) <= 0 && mg_get_http_var(&(hm->query_string), "g", possiblePix.g, 4) <= 0)
        return 0; 
    //printf("g: %s\n", possiblePix.g);
    if (mg_get_http_var(&(hm->body), "b", possiblePix.b, 4) <= 0 && mg_get_http_var(&(hm->query_string), "b", possiblePix.b, 4) <= 0)
        return 0; 
    //printf("b: %s\n", possiblePix.b);

    pxl->x = atoi(possiblePix.x);
    pxl->y = atoi(possiblePix.y);
    pxl->r = atoi(possiblePix.r);
    pxl->g = atoi(possiblePix.g);
    pxl->b = atoi(possiblePix.b);

    if (goodBounds(pxl->x, 128) && goodBounds(pxl->y, 128) && goodBounds(pxl->r, 256) && goodBounds(pxl->g, 256) && goodBounds(pxl->b, 256)) {
        return 1;
    } 

    return 0;
}

void prepend0(char* s) {
    memmove(s+1,s,strlen(s)+1);
    s[0] = '0';
}

char* toHex(char* b, int a) {
    sprintf(b, "%x", a);

    if (strlen(b) < 2)
        prepend0(b);

    return b;
}

void drawPixel(struct mg_connection *nc, struct pixel pxl) {
    char* x = new char[3]; 
    char* y = new char[3]; 
    char* r = new char[3]; 
    char* g = new char[3]; 
    char* b = new char[3];

    toHex(x, pxl.x);
    toHex(y, pxl.y);
    toHex(r, pxl.r);
    toHex(g, pxl.g);
    toHex(b, pxl.b);

    char* full = strcat(strcat(strcat(strcat(strcpy(new char[11],x),y),r),g),b);

    *(board+6*(pxl.x*128+pxl.y)) = r[0];
    *(board+6*(pxl.x*128+pxl.y)+1) = r[1];
    *(board+6*(pxl.x*128+pxl.y)+2) = g[0];
    *(board+6*(pxl.x*128+pxl.y)+3) = g[1];
    *(board+6*(pxl.x*128+pxl.y)+4) = b[0];
    *(board+6*(pxl.x*128+pxl.y)+5) = b[1];

    //printf("%s\n", board);
    
    struct mg_connection *c;
    for (c = mg_next(nc->mgr, NULL); c!= NULL; c = mg_next(nc->mgr, c)) {
        mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, full, strlen(full));
        //printf("a\n");
    }
}

static void post_handler(struct mg_connection *nc, int ev, void *p) {
    if (ev == MG_EV_HTTP_REQUEST) {
        struct http_message *hm = (struct http_message *) p;

        struct pixel pxl;

        int result = parsePixelRequest(hm, &pxl);

        if (result == 1) {
            mg_send_head(nc, 200, strlen("pixel recieved\n"), "Content-Type: text/plain");
            mg_printf(nc, "pixel recieved\n"/*, (int)hm->message.len, hm->message.p*/);

            drawPixel(nc, pxl);
        } else {
            mg_send_head(nc, 400, strlen("not enough fields recieved or values sent not in bounds\n"), "Content-Type: text/plain");
            mg_printf(nc, "not enough fields recieved or values sent not in bounds\n"/*, (int)hm->message.len, hm->message.p*/);
        }
    } 
}

static void live_handler(struct mg_connection *nc, int ev, void *p) {
    if (ev == MG_EV_HTTP_REQUEST) {
        struct http_message *hm = (struct http_message *) p;
        mg_http_serve_file(nc, hm, "client/index.html", mg_mk_str("text/html"), mg_mk_str(""));
    } else if (ev == MG_EV_WEBSOCKET_HANDSHAKE_DONE) {   
        printf("new websocket connection\n");
        mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, board, strlen(board));        
    }  
}

static void js_server(struct mg_connection *nc, int ev, void *p) {
    struct http_message *hm = (struct http_message *) p;
    mg_http_serve_file(nc, hm, "client/script.js", mg_mk_str("text/js"), mg_mk_str("")); 
}

int main(void) {
    for (int i = 0; i <= 6*128*128; i++) {
        board[i] = '0';
    }

    struct mg_mgr mgr;
    struct mg_connection *c;

    mg_mgr_init(&mgr, NULL);
    c = mg_bind(&mgr, s_http_port, post_handler);
    mg_register_http_endpoint(c, "/live", live_handler);
    mg_register_http_endpoint(c, "/script.js", js_server);
    mg_set_protocol_http_websocket(c);

    while (1) {
        mg_mgr_poll(&mgr, 1000);
    }
    mg_mgr_free(&mgr);

    return 0;
}

