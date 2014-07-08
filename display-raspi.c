#include "display.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <bcm_host.h>
#include <GLES/gl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>


#include "config.h"
#include "color.h"
#include "simulation.h"

typedef struct
{
   rgb color;
   float fadeout;
} UiPlayer;

static UiPlayer* uiPlayer;

static int fps;
static uint32_t screenW, screenH;
static float left, right, bottom, top, zoom;

static EGLDisplay display;
static EGLSurface surface;
static EGLContext context;

float renderBuffer[3*2000];

static void init_ogl()
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
   success = graphics_get_display_size(0, &screenW, &screenH);
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
}

static void reshape()
{
   double ratioScreen = (double)screenW / screenH;
   double ratioSim = conf.battlefieldW / conf.battlefieldH;
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
   int p;

   bcm_host_init();
   init_ogl();
   reshape();

   uiPlayer = malloc(conf.maxPlayers * sizeof(UiPlayer));
   for(p = 0; p < conf.maxPlayers; ++p)
   {
      hsv color;
      color.h = 360.0 / conf.maxPlayers * p;
      color.s = 0.8;
      color.v = 1.0;
      uiPlayer[p].color = hsv2rgb(color);
   }
   zoom = 0.8;
}

static void drawFps(void)
{
/*
   static unsigned long timeOld;
   static int frameCounter;
   static double dfps;

   unsigned long time;
   char buffer[16];

   frameCounter++;
   time = glutGet(GLUT_ELAPSED_TIME);

   if(time > timeOld + 500)
   {
      dfps = frameCounter * 1000.0 / (time - timeOld);

      frameCounter = 0;
      timeOld = time;
   }

   sprintf(buffer, "%.1lf fps", dfps);

   glColor3f(1.0, 1.0, 1.0);
   glRasterPos2d(3.0, 30.0);
   glutBitmapString(GLUT_BITMAP_9_BY_15, (unsigned char*)buffer);
*/
}

static void drawPlayers(void)
{
/*
   static char buffer[128];
   int p;

   for(p = 0; p < conf.maxPlayers; ++p)
   {
      SimPlayer* pl = getPlayer(p);
      if(!pl->active) continue;
      glColor3d(uiPlayer[p].color.r, uiPlayer[p].color.g, uiPlayer[p].color.b);

      sprintf(buffer, "%s%s (%d:%d)%s", p == getCurrentPlayer() ? "> " : "  ", pl->name, pl->kills, pl->deaths, p == getCurrentPlayer() ? " <" : "");
      if(conf.oneline)
      {
         glRasterPos2d(p * screenW / conf.maxPlayers + 3.0, 15.0);
      }
      else
      {
         glRasterPos2d((p / 2) * (screenW / ((conf.maxPlayers + 1) / 2)) + 3.0, (p % 2) ? screenH - 6.0 : 15.0);
      }
      glutBitmapString(GLUT_BITMAP_9_BY_15, (unsigned char*)buffer);
   }
*/
}

void stepDisplay(void)
{
   int i, p, s;

   glViewport(0, 0, screenW, screenH);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrthof(left, right, bottom, top, -100, 100); /* left, right, bottom, top, near, far */

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
      //glutWireSphere(4.0, 16, 2);
      glPopMatrix();
   }

   glColor4f(0.3, 0.3, 0.3, 1.0);
   for(i = 0; i < conf.numPlanets; ++i)
   {
      SimPlanet* p = getPlanet(i);
      glPushMatrix();
      glTranslatef(p->position.x, p->position.y, 0);
      //glutWireSphere(p->radius, 16, 2);
      glPopMatrix();
   }

   #if 0
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, screenW, screenH, 0, -100, 100); /* left, right, bottom, top, near, far */

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   if(fps) drawFps();
   drawPlayers();

   glFlush();
   glutSwapBuffers();
   #endif

   eglSwapBuffers(display, surface);
}

void updateZoom(double z)
{
   zoom = LIMIT(z, 0.0, 1.0);
}

void toggleFps(void)
{
   fps = !fps;
}
