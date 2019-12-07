#include "pixel.h"
#include "server.h"

unsigned int scale = 128;
unsigned int width = 640;
unsigned int height = 480;
float targetX;
float targetY;

struct Pixel ** pixels;

std::queue<struct Request*> requestQueue;
std::mutex queueMutex;
std::mutex runningMutex;

time_t timer;
unsigned int deltaTime = 0;
FILE* output;

void update()
{   
    time_t now = time(NULL);
    deltaTime += now - timer;
    timer = time(NULL);
    queueMutex.lock();
    if (requestQueue.size() > 0)
    {
        printf("Popping queue\n");
        struct Request** array = new struct Request*[requestQueue.size()];
        unsigned int size = requestQueue.size();
        for (int i = 0; i < size; i++)
        {
            array[i] = requestQueue.front();
            requestQueue.pop();
        }
        queueMutex.unlock();
        for (int i = 0; i < size; i++)
        {
            pixels[array[i]->y][array[i]->x].r = array[i]->r;
            pixels[array[i]->y][array[i]->x].g = array[i]->g;
            pixels[array[i]->y][array[i]->x].b = array[i]->b;
        }
        // pixels[r->y][r->x].r = r->r;
        // pixels[r->y][r->x].g = r->g;
        // pixels[r->y][r->x].b = r->b;
        for (int i = 0; i < size; i++)
        {
            delete[] array[i];
        }
        delete[] array;
        glutPostRedisplay();
    } else 
    {
        queueMutex.unlock();
        if (deltaTime >= 60 * 1)
        {
            deltaTime = 0;
            printf("Starting JPEG write\n");
            output = fopen("state.jpg", "wb+");
			if (output == NULL)
			{
				printf("Error opening image file!\n");
				return;
			}

			struct jpeg_compress_struct cinfo;
			struct jpeg_error_mgr error;
			JSAMPROW rowPointer[1];

			cinfo.err = jpeg_std_error(&error);
			jpeg_create_compress(&cinfo);
			jpeg_stdio_dest(&cinfo, output);

			cinfo.image_width = scale;
			cinfo.image_height = scale;
			
			cinfo.input_components = 3;
			cinfo.in_color_space = JCS_RGB;

			jpeg_set_defaults(&cinfo);
			jpeg_set_quality(&cinfo, 100, TRUE);

			jpeg_start_compress(&cinfo, TRUE);
			
			for (unsigned int y = 0; y < scale; y++)
			{
                unsigned char * image = new unsigned char[3 * scale];
                for (int x = 0; x < scale; x++)
                {
                    image[x * 3] = pixels[y][x].r;
                    image[x * 3 + 1] = pixels[y][x].g;
                    image[x * 3 + 2] = pixels[y][x].b;
                }
				rowPointer[0] = image;
				jpeg_write_scanlines(&cinfo, rowPointer, 1);
                delete[] image;
			}
			
			jpeg_finish_compress(&cinfo);
			jpeg_destroy_compress(&cinfo);
			printf("Finished Writing!\n");
			if (fclose(output) != 0)
			{
				printf("Failed to close image\n");
			}
		}
	}
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_QUADS);
    for (int y = 0; y < scale; y++)
        for (int x = 0; x < scale; x++)
        {
            glColor3ub(pixels[y][x].r, pixels[y][x].g, pixels[y][x].b);
            glVertex2f(pixels[y][x].startX, pixels[y][x].startY);
            glVertex2f(pixels[y][x].endX, pixels[y][x].startY);
            glVertex2f(pixels[y][x].endX, pixels[y][x].endY);
            glVertex2f(pixels[y][x].startX, pixels[y][x].endY);
        }
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        printf("OpenGL error: %s\n", gluErrorString(err));
    }
    glEnd();
    glutSwapBuffers();
}

int main(int argc, char** argv)
{
    targetX = (float) width / scale;
    targetY = (float) height / scale;
    printf("TargetX = %f, TargetY = %f\n", targetX, targetY);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutInitWindowSize(width, height);
    glutInitWindowPosition(0, 0);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
    glutCreateWindow("ACM Pixel");
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, width, height, 0);
    //Disable unused GL functions
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_1D);
    glDisable(GL_LIGHTING);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
	
    pixels = new struct Pixel*[scale];
    for (int y = 0; y < scale; y++)
        pixels[y] = new struct Pixel[scale];
    
    for (int y = 0; y < scale; y++)
        for (int x = 0; x < scale; x++)
        {
            pixels[y][x].startX = x * targetX;
            pixels[y][x].startY = y * targetY;
            pixels[y][x].endX = pixels[y][x].startX + targetX;
            pixels[y][x].endY = pixels[y][x].startY + targetY;
            pixels[y][x].r = 0;
            pixels[y][x].g = 0;
            pixels[y][x].b = 0;
        }
    
    requestQueue = std::queue<struct Request*>();
	startNetworkThread();
    glutDisplayFunc(render);
    glutIdleFunc(update);
    glutMainLoop();
    runningMutex.lock();
    runningMutex.unlock();
	stopNetworkThread();
    for (int y = 0; y < scale; y++)
        delete[] pixels[y];
    delete[] pixels;
    return 0;
}
