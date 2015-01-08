#include "display.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>

#if defined TARGET_GLUT
   #if defined(__MACH__)
      #include <OpenGL/gl.h>
      #include <OpenGL/glu.h>
      #include <GLUT/glut.h>
      #include <OpenGL/glext.h>
   #elif
      #include <GL/freeglut.h>
   #endif
#elif defined TARGET_RASPI
   #include <bcm_host.h>
   #include <GLES/gl.h>
   #include <EGL/egl.h>
   #include <EGL/eglext.h>
   #include <sys/time.h>
   #include <time.h>
#endif

#include "config.h"
#include "color.h"
#include "simulation.h"

#if defined TARGET_GLUT
   #ifndef GL_CLAMP_TO_EDGE
   #define GL_CLAMP_TO_EDGE  0x812F
   #endif
#elif defined TARGET_RASPI
   #define glOrtho glOrthof
#endif

#define MIN(x, max) ((x) > (max) ? (max) : (x))

static void initSystem(int* argc, char** argv);
static void swapBuffers(void);
static unsigned long getTime(void);

typedef struct
{
   rgb color;
   float fadeout;
} UiPlayer;

static UiPlayer* uiPlayer;
static GLuint tex;
static float vertCircle[32][2];
static float left, right, bottom, top, zoom;
static int screenW, screenH;
static float uiW, uiH;
static int fps;

static void drawString(char* str, float x, float y, float r, float g, float b)
{
   const char *p;

   glEnable(GL_TEXTURE_2D);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   glBindTexture(GL_TEXTURE_2D, tex);

   glColor4f(r, g, b, 1.0);

   /* Loop through all characters */
   for (p = str; *p; ++p)
   {
      /* Calculate the vertex and texture coordinates */
      float vert[] =
      {
         x,        y - 24.0,
         x + 16.0, y - 24.0,
         x,        y,
         x + 16.0, y,
      };
      float texc[] =
      {
         (*p - 32) / 96.0, 0.0,
         (*p - 31) / 96.0, 0.0,
         (*p - 32) / 96.0, 1.0,
         (*p - 31) / 96.0, 1.0,
      };

      /* Draw the character on the screen */
      glVertexPointer(2, GL_FLOAT, 0, vert);
      glTexCoordPointer(2, GL_FLOAT, 0, texc);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

      /* Advance the cursor to the start of the next character */
      x += 14.0;
   }
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
   glDisable(GL_TEXTURE_2D);
}

static void drawJoin()
{
   char buffer[128];
   sprintf(buffer, "To play, telnet %s 3490", conf.ip);
   drawString(buffer, 3.0, 24.0, 1.0, 1.0, 1.0);
}

static void drawFps(int offset)
{
   static unsigned long timeOld;
   static int frameCounter;
   static float dfps;

   unsigned long time;
   char buffer[16];

   frameCounter++;
   time = getTime();

   if(time > timeOld + 500)
   {
      dfps = frameCounter * 1000.0 / (time - timeOld);
      frameCounter = 0;
      timeOld = time;
   }

   sprintf(buffer, "%.1lf fps", dfps);
   drawString(buffer, 3.0, (2 + offset) * 24.0, 1.0, 1.0, 1.0);
}

static void drawPlayers(int offset, int activeP)
{
   static char buffer[128];
   float x, y;
   int p;

   for(p = 0; p < conf.maxPlayers; ++p)
   {
      SimPlayer* pl = getPlayer(p);
      if(!pl->active) continue;

      x = (p % 6) * uiW / MIN(conf.maxPlayers, 6) + 3.0;
      y = (1 + offset) * 24.0;
      if (p / 6) y = uiH - 3.0;
      sprintf(buffer, "%s%s (%d:%d)%s", p == getCurrentPlayer() ? "> " : "  ", pl->name, pl->kills, pl->deaths, p == getCurrentPlayer() ? " <" : "");
      drawString(buffer, x, y, uiPlayer[p].color.r, uiPlayer[p].color.g, uiPlayer[p].color.b);

      if(conf.timeout && activeP > 1)
      {
         x = (p % 6) * uiW / MIN(conf.maxPlayers, 6) + 3.0;
         y = (2 + offset) * 24.0;
         if (p / 6) y = uiH - 3.0 - 24.0;
         sprintf(buffer, "  %.0lf", pl->timeout / 60.0);
         drawString(buffer, x, y, uiPlayer[p].color.r, uiPlayer[p].color.g, uiPlayer[p].color.b);
      }
   }
}

static void draw(void)
{
   int i, p, s, actp;
   
   for(i = 0, actp = 0; i < conf.maxPlayers; ++i) actp += getPlayer(i)->active;

   glViewport(0, 0, screenW, screenH);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(left, right, bottom, top, -100, 100); /* left, right, bottom, top, near, far */

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   glClearColor(getFlash(), getFlash(), getFlash(), 1.0);
   glClear(GL_COLOR_BUFFER_BIT);

   glTranslatef(conf.battlefieldW / 2, conf.battlefieldH / 2, 0);
   glScalef(zoom, zoom, zoom);
   glTranslatef(-conf.battlefieldW / 2, -conf.battlefieldH / 2, 0);

   glColor4f(0.3, 0.3, 0.3, 1.0);
   for(i = 0; i < conf.numPlanets; ++i)
   {
      SimPlanet* p = getPlanet(i);
      glPushMatrix();
      glTranslatef(p->position.x, p->position.y, 0);
      glScalef(p->radius, p->radius, 1.0);
      glVertexPointer(2, GL_FLOAT, 0, vertCircle);
      glDrawArrays(GL_LINE_LOOP, 0, 16);
      glDrawArrays(GL_LINES, 16, 16);
      glPopMatrix();
   }

   for(p = 0; p < conf.maxPlayers; ++p)
   {
      SimPlayer* pl = getPlayer(p);
      if(!pl->active) continue;
      uiPlayer[p].fadeout = LIMIT(uiPlayer[p].fadeout - 0.02, 0.0, 1.0);
      for(s = conf.numShots - 1; s >= 0; --s)
      {
         double bright;
         SimShot* shot;
         shot = getShot(p, s);
         if(shot->missile.live && shot->length < 4)
         {
            uiPlayer[p].fadeout = 1.0;
         }
         bright = (double)(conf.numShots - s - 1) / (conf.numShots - 1);
         if(s > 0)
         {
            bright += uiPlayer[p].fadeout / (conf.numShots - 1);
         }
         glColor4f(uiPlayer[p].color.r, uiPlayer[p].color.g, uiPlayer[p].color.b, bright);
         glVertexPointer(2, GL_FLOAT, 0, shot->dot);
         glDrawArrays(GL_LINE_STRIP, 0, shot->length);
         if(shot->missile.live && shot->length > 2)
         {
            glColor4f(1.0, 1.0, 1.0, bright);
            glVertexPointer(2, GL_FLOAT, 0, shot->dot + shot->length - 3);
            glDrawArrays(GL_LINE_STRIP, 0, 3);
         }
      }
      glColor4f(uiPlayer[p].color.r, uiPlayer[p].color.g, uiPlayer[p].color.b, 1.0f);
      glPushMatrix();
      glTranslatef(pl->position.x, pl->position.y, 0);
      glScalef(4.0, 4.0, 1.0);
      glVertexPointer(2, GL_FLOAT, 0, vertCircle);
      glDrawArrays(GL_LINE_LOOP, 0, 16);
      glDrawArrays(GL_LINES, 16, 16);
      if(p == getCurrentPlayer() && actp > 1)
      {
         glScalef(4.0, 4.0, 1.0);
         glDrawArrays(GL_LINE_LOOP, 0, 16);
      }
      glPopMatrix();
   }

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();

   glOrtho(0, uiW, uiH, 0, -100, 100); /* left, right, bottom, top, near, far */

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   s = ((actp < conf.maxPlayers) && (conf.ip != 0));

   if(s) drawJoin();
   drawPlayers(s, actp);
   if(conf.timeout && actp > 1) s++;
   if(fps) drawFps(s);

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
   uiW = screenW > 1600 ? screenW : screenW < 800 ? screenW * 2 : 1600;
   uiH = uiW * screenH / screenW;
}

void initDisplay(int* argc, char** argv)
{
   int p, i;
   GLuint font[96*24*16*4];
   FILE* fp;

   initSystem(argc, argv);

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glLineWidth(2.0);
   glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
   glEnable(GL_LINE_SMOOTH);
   glEnableClientState(GL_VERTEX_ARRAY);

   uiPlayer = malloc(conf.maxPlayers * sizeof(UiPlayer));

   for(p = 0; p < conf.maxPlayers; ++p)
   {
      hsv color;
      color.h = 360.0 / MIN(conf.maxPlayers, 6) * p + (p / 6) * 30.0;
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
      vertCircle[8 + i][0] = -x;
      vertCircle[8 + i][1] = -y;
      vertCircle[16 + 2 * i][0] = x;
      vertCircle[16 + 2 * i][1] = y;
      vertCircle[16 + 2 * i + 1][0] = -x;
      vertCircle[16 + 2 * i + 1][1] = -y;
   }
   
   fp = fopen("font24.raw", "rb");
   assert(fread(font, 1, 96*24*16*4, fp) == 96*24*16*4);
   fclose(fp);
   
   /* Create a texture that will hold the font */
   glGenTextures(1, &tex);
   glBindTexture(GL_TEXTURE_2D, tex);

   /* Clamping to edges is important to prevent artifacts when scaling */
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   /* Linear filtering usually looks best for text */
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   
   /* Upload the font */
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 96*16, 24, 0, GL_RGBA, GL_UNSIGNED_BYTE, font);
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
   glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
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
}

static void swapBuffers()
{
   glutSwapBuffers();
}

static unsigned long getTime(void)
{
   return glutGet(GLUT_ELAPSED_TIME);
}

void stepDisplay(void)
{
   glutPostRedisplay();
#if defined(__MACH__)
   glutCheckLoop();
#elif
   glutMainLoopEvent();
#endif
}

#elif defined TARGET_RASPI

static EGLDisplay display;
static EGLSurface surface;
static EGLContext context;

void initSystem(int* argc, char** argv)
{
   uint32_t w, h;
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
   success = graphics_get_display_size(0, &w, &h);
   assert(success >= 0);

   dst_rect.x = 0;
   dst_rect.y = 0;
   dst_rect.width = w;
   dst_rect.height = h;

   src_rect.x = 0;
   src_rect.y = 0;
   src_rect.width = w << 16;
   src_rect.height = h << 16;

   dispman_display = vc_dispmanx_display_open(0);
   dispman_update = vc_dispmanx_update_start(0);

   dispman_element = vc_dispmanx_element_add(dispman_update, dispman_display, 0/*layer*/, &dst_rect, 0/*src*/,
      &src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, 0/*transform*/);

   nativewindow.element = dispman_element;
   nativewindow.width = w;
   nativewindow.height = h;
   vc_dispmanx_update_submit_sync(dispman_update);

   surface = eglCreateWindowSurface(display, config, &nativewindow, NULL);
   assert(surface != EGL_NO_SURFACE);

   // connect the context to the surface
   result = eglMakeCurrent(display, surface, surface, context);
   assert(EGL_FALSE != result);

   reshape(w, h);
}

void swapBuffers(void)
{
   eglSwapBuffers(display, surface);
}

static unsigned long getTime(void)
{
   struct timeval tv;
   gettimeofday(&tv, 0);
   return 1000 * tv.tv_sec + tv.tv_usec / 1000;
}

void stepDisplay(void)
{
   draw();
}
#endif
