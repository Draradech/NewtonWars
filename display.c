#include "display.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#if defined TARGET_GLUT
#include <GL/freeglut.h>
#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif
#elif defined TARGET_RASPI
#include <assert.h>
#include <bcm_host.h>
#include <GLES/gl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <sys/time.h>
#include <time.h>
#endif

#include <ft2build.h>
#include FT_FREETYPE_H

#include "config.h"
#include "color.h"
#include "simulation.h"

#if defined TARGET_RASPI
#define glOrtho glOrthof
#endif

static void initSystem(int* argc, char** argv);
static void drawString(char* str, float x, float y, int size, float r, float g, float b);
static void swapBuffers(void);

typedef struct
{
   rgb color;
   float fadeout;
} UiPlayer;

static UiPlayer* uiPlayer;
static float* renderBuffer;
static FT_Face face;
static float vertCircle[32][3];
static float left, right, bottom, top, zoom;
static int screenW, screenH;
static int fps;

#if defined TARGET_RASPI
static EGLDisplay display;
static EGLSurface surface;
static EGLContext context;
#endif

static GLubyte transformer[1000000];

static void drawString(char* str, float x, float y, int size, float r, float g, float b)
{
   int gy, gx;
   const char *p;
   FT_GlyphSlot gl = face->glyph;

   FT_Set_Pixel_Sizes(face, 0, size *16);
   
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);

   /* Create a texture that will be used to hold one "glyph" */
   GLuint tex;
   glActiveTexture(GL_TEXTURE0);
   glGenTextures(1, &tex);
   glBindTexture(GL_TEXTURE_2D, tex);

   /* We require 1 byte alignment when uploading texture data */
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   /* Clamping to edges is important to prevent artifacts when scaling */
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   /* Linear filtering usually looks best for text */
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   
   glColor4f(r, g, b, 1.0);

   /* Loop through all characters */
   for (p = str; *p; p++)
   {
      /* Try to load and render the character */
      if (FT_Load_Char(face, *p, FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT))
      {
         continue;
      }
      
      for(gy = 0; gy < gl->bitmap.rows; ++gy)
      {
         for(gx = 0; gx < gl->bitmap.width; ++gx)
         {
            transformer[(gy * gl->bitmap.width + gx) * 4 + 0] = gl->bitmap.buffer[gy * gl->bitmap.width + gx];
            transformer[(gy * gl->bitmap.width + gx) * 4 + 1] = gl->bitmap.buffer[gy * gl->bitmap.width + gx];
            transformer[(gy * gl->bitmap.width + gx) * 4 + 2] = gl->bitmap.buffer[gy * gl->bitmap.width + gx];
            transformer[(gy * gl->bitmap.width + gx) * 4 + 3] = gl->bitmap.buffer[gy * gl->bitmap.width + gx];
         }
      }
      
      /* Upload the "bitmap", which contains an 8-bit grayscale image, as an alpha texture */
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gl->bitmap.width, gl->bitmap.rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, transformer);
      
      /* Calculate the vertex and texture coordinates */
      float x2 = x + gl->bitmap_left/16.0;
      float y2 = y - gl->bitmap_top/16.0;
      float w = gl->bitmap.width/16.0;
      float h = gl->bitmap.rows/16.0;
      float vert[] =
      {
         x2, y2,
         x2 + w, y2,
         x2, y2 + h,
         x2 + w, y2 + h,
      };
      float texc[] =
      {
         0, 0,
         1, 0,
         0, 1,
         1, 1,
      };
      
      /* Draw the character on the screen */
      glVertexPointer(2, GL_FLOAT, 0, vert);
      glTexCoordPointer(2, GL_FLOAT, 0, texc);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      
      /* Advance the cursor to the start of the next character */
      x += (gl->advance.x/16.0 /64);
      y += (gl->advance.y/16.0 /64);
   }
   glDeleteTextures(1, &tex);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

static void drawFps(void)
{
   static unsigned long timeOld;
   static int frameCounter;
   static float dfps;

   unsigned long time;
   #if defined TARGET_RASPI
   struct timeval tv;
   #endif
   char buffer[16];

   frameCounter++;
   #if defined TARGET_RASPI
   gettimeofday(&tv, 0);
   time = 1000 * tv.tv_sec + tv.tv_usec / 1000;
   #elif defined TARGET_GLUT
   time = glutGet(GLUT_ELAPSED_TIME);
   #endif

   if(time > timeOld + 500)
   {
      dfps = frameCounter * 1000.0 / (time - timeOld);

      frameCounter = 0;
      timeOld = time;
   }

   sprintf(buffer, "%.1lf fps", dfps);

   drawString(buffer, 3.0, 30.0, 18, 1.0, 1.0, 1.0);
}

static void drawPlayers(void)
{
   static char buffer[128];
   float x, y;
   int p;

   for(p = 0; p < conf.maxPlayers; ++p)
   {
      SimPlayer* pl = getPlayer(p);
      if(!pl->active) continue;

      sprintf(buffer, "%s%s (%d:%d)%s", p == getCurrentPlayer() ? "> " : "  ", pl->name, pl->kills, pl->deaths, p == getCurrentPlayer() ? " <" : "");
      if(conf.oneline)
      {
         x = p * (float)screenW / conf.maxPlayers + 3.0;
         y = 15.0;
      }
      else
      {
         x = (p / 2) * ((float)screenW / ((conf.maxPlayers + 1) / 2)) + 3.0;
         y = (p % 2) ? screenH - 6.0 : 15.0;
      }
      drawString(buffer, x, y, 18, uiPlayer[p].color.r, uiPlayer[p].color.g, uiPlayer[p].color.b);
   }
}

static void draw(void)
{
   int i, p, s;

   glViewport(0, 0, screenW, screenH);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(left, right, bottom, top, -100, 100); /* left, right, bottom, top, near, far */

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   glClear(GL_COLOR_BUFFER_BIT);

   glTranslatef((right - left) / 2, (bottom - top) / 2, 0);
   glScalef(zoom, zoom, zoom);
   glTranslatef((left - right) / 2, (top - bottom) / 2, 0);

   glEnableClientState(GL_VERTEX_ARRAY);

   for(p = 0; p < conf.maxPlayers; ++p)
   {
      SimPlayer* pl = getPlayer(p);
      if(!pl->active) continue;
      uiPlayer[p].fadeout = LIMIT(uiPlayer[p].fadeout - 0.02, 0.0, 1.0);
      for(s = 0; s < conf.numShots; ++s)
      {
         double bright;
         SimShot* shot;
         shot = getShot(p, s);
         if(shot->missile.live && shot->length < 4)
         {
            uiPlayer[p].fadeout = 1.0;
         }
         bright = (double)(conf.numShots - s) / conf.numShots;
         if(s > 0)
         {
            bright += uiPlayer[p].fadeout / conf.numShots;
         }
         bright *= bright;
         for(i = 0; i < shot->length; ++i)
         {
            renderBuffer[i*3] = shot->dot[i].x;
            renderBuffer[i*3+1] = shot->dot[i].y;
            renderBuffer[i*3+2] = 0;
         }
         glColor4f(uiPlayer[p].color.r, uiPlayer[p].color.g, uiPlayer[p].color.b, bright);
         glVertexPointer(3, GL_FLOAT, 0, renderBuffer);
         glDrawArrays(GL_LINE_STRIP, 0, shot->length);
      }
      glColor4f(uiPlayer[p].color.r, uiPlayer[p].color.g, uiPlayer[p].color.b, 1.0f);
      glPushMatrix();
      glTranslatef(pl->position.x, pl->position.y, 0);
      glScalef(4.0, 4.0, 1.0);
      glVertexPointer(3, GL_FLOAT, 0, vertCircle);
      glDrawArrays(GL_LINE_LOOP, 0, 16);
      glDrawArrays(GL_LINES, 16, 16);
      glPopMatrix();
   }

   glColor4f(0.3, 0.3, 0.3, 1.0);
   for(i = 0; i < conf.numPlanets; ++i)
   {
      SimPlanet* p = getPlanet(i);
      glPushMatrix();
      glTranslatef(p->position.x, p->position.y, 0);
      glScalef(p->radius, p->radius, 1.0);
      glVertexPointer(3, GL_FLOAT, 0, vertCircle);
      glDrawArrays(GL_LINE_LOOP, 0, 16);
      glDrawArrays(GL_LINES, 16, 16);
      glPopMatrix();
   }

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, screenW, screenH, 0, -100, 100); /* left, right, bottom, top, near, far */

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   if(fps) drawFps();
   drawPlayers();

   swapBuffers();
}

static void reshape(int w, int h)
{
   double ratioScreen = (double)w / h;
   double ratioSim = conf.battlefieldW / conf.battlefieldH;
   screenW = w;
   screenH = h;
   if(ratioScreen > ratioSim)
   {
      top = 0.0;
      bottom = conf.battlefieldH;
      left = -(conf.battlefieldH * ratioScreen - conf.battlefieldW) / 2.0;
      right = conf.battlefieldW + (conf.battlefieldH * ratioScreen - conf.battlefieldW) / 2.0;
   }
   else
   {
      top = -(conf.battlefieldW / ratioScreen - conf.battlefieldH) / 2.0;
      bottom = conf.battlefieldH + (conf.battlefieldW / ratioScreen - conf.battlefieldH) / 2.0;
      left = 0.0;
      right = conf.battlefieldW;
   }
}

void initDisplay(int* argc, char** argv)
{
   int p, i;

   initSystem(argc, argv);
   glEnable(GL_BLEND);
   glEnable(GL_TEXTURE_2D);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glClearColor(0.0, 0.0, 0.0, 1.0);

   FT_Library ft;

   if(FT_Init_FreeType(&ft))
   {
      fprintf(stderr, "Could not init freetype library\n");
   }

   if(FT_New_Face(ft, "UbuntuMono-B.ttf", 0, &face))
   {
      fprintf(stderr, "Could not open font\n");
   }
   
   uiPlayer = malloc(conf.maxPlayers * sizeof(UiPlayer));
   renderBuffer = malloc(conf.maxSegments * 3 * sizeof(float));

   for(p = 0; p < conf.maxPlayers; ++p)
   {
      hsv color;
      color.h = 360.0 / conf.maxPlayers * p;
      color.s = 0.8;
      color.v = 1.0;
      uiPlayer[p].color = hsv2rgb(color);
   }
   zoom = 0.8;

   for(i = 0; i < 8; ++i)
   {
      double a = 2 * M_PI / 32 + 2 * M_PI / 16 * i;
      float x = sin(a);
      float y = cos(a);
      vertCircle[i][0] = x;
      vertCircle[i][1] = y;
      vertCircle[i][2] = 0.0;
      vertCircle[8 + i][0] = -x;
      vertCircle[8 + i][1] = -y;
      vertCircle[8 + i][2] = 0.0;
      vertCircle[16 + 2 * i][0] = x;
      vertCircle[16 + 2 * i][1] = y;
      vertCircle[16 + 2 * i][2] = 0.0;
      vertCircle[16 + 2 * i + 1][0] = -x;
      vertCircle[16 + 2 * i + 1][1] = -y;
      vertCircle[16 + 2 * i + 1][2] = 0.0;
   }
}

void updateZoom(double z)
{
   zoom = LIMIT(z, 0.0, 1.0);
}

void toggleFps(void)
{
   fps = !fps;
}

#if defined TARGET_GLUT
static void initSystem(int* argc, char ** argv)
{
   glutInit(argc, argv);
   glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
   glutCreateWindow("NewtonWars");

   if(conf.fullscreen)
   {
      glutFullScreen();
   }
   else
   {
      glutPositionWindow(0, 0);
      glutReshapeWindow(800, 600);
   }

   glutDisplayFunc(draw);
   glutReshapeFunc(reshape);

   glEnable(GL_MULTISAMPLE);
}

static void drawString15(char* str, float x, float y, float r, float g, float b)
{
   glColor3d(r, g, b);
   glRasterPos2d(x, y);
   glutBitmapString(GLUT_BITMAP_9_BY_15, (unsigned char*)str);
}

static void swapBuffers()
{
   glFlush();
   glutSwapBuffers();
}

void stepDisplay(void)
{
   glutPostRedisplay();
   glutMainLoopEvent();
}

#elif defined TARGET_RASPI

void initSystem(int* argc, char** argv)
{
   int success = 0;
   EGLBoolean result;
   EGLint num_config;

   static EGL_DISPMANX_WINDOW_T nativewindow;

   DISPMANX_ELEMENT_HANDLE_T dispman_element;
   DISPMANX_DISPLAY_HANDLE_T dispman_display;
   DISPMANX_UPDATE_HANDLE_T dispman_update;
   VC_RECT_T dst_rect;
   VC_RECT_T src_rect;

   static const EGLint attribute_list[] =
   {
      EGL_RED_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_BLUE_SIZE, 8,
      EGL_ALPHA_SIZE, 8,
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
      EGL_NONE
   };

   EGLConfig config;

   bcm_host_init();

   // get an EGL display connection
   display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
   assert(display!=EGL_NO_DISPLAY);

   // initialize the EGL display connection
   result = eglInitialize(display, NULL, NULL);
   assert(EGL_FALSE != result);

   // get an appropriate EGL frame buffer configuration
   result = eglChooseConfig(display, attribute_list, &config, 1, &num_config);
   assert(EGL_FALSE != result);

   // create an EGL rendering context
   context = eglCreateContext(display, config, EGL_NO_CONTEXT, NULL);
   assert(context!=EGL_NO_CONTEXT);

   // create an EGL window surface
   success = graphics_get_display_size(0, (uint32_t*)&screenW, (uint32_t*)&screenH);
   assert(success >= 0);

   dst_rect.x = 0;
   dst_rect.y = 0;
   dst_rect.width = screenW;
   dst_rect.height = screenH;

   src_rect.x = 0;
   src_rect.y = 0;
   src_rect.width = screenW << 16;
   src_rect.height = screenH << 16;

   dispman_display = vc_dispmanx_display_open(0);
   dispman_update = vc_dispmanx_update_start(0);

   dispman_element = vc_dispmanx_element_add(dispman_update, dispman_display, 0/*layer*/, &dst_rect, 0/*src*/,
      &src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, 0/*transform*/);

   nativewindow.element = dispman_element;
   nativewindow.width = screenW;
   nativewindow.height = screenH;
   vc_dispmanx_update_submit_sync(dispman_update);

   surface = eglCreateWindowSurface(display, config, &nativewindow, NULL);
   assert(surface != EGL_NO_SURFACE);

   // connect the context to the surface
   result = eglMakeCurrent(display, surface, surface, context);
   assert(EGL_FALSE != result);

   reshape(screenW, screenH);
}

static void drawString15(char* str, float x, float y, float r, float g, float b)
{
}

void swapBuffers(void)
{
   eglSwapBuffers(display, surface);
}

void stepDisplay(void)
{
   draw();
}
#endif
