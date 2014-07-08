#include "simulation.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <GL/freeglut.h>

#include "config.h"
#include "color.h"

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

typedef struct
{
   rgb color;
   double fadeout;
} UiPlayer;

static UiPlayer* uiPlayer;

static double left, right, bottom, top, zoom;
static int screenW, screenH;
static int fps;

static void drawFps(void)
{
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
}

static void drawPlayers(void)
{
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
}

static void display(void)
{
   int i, p, s;

   glViewport(0, 0, screenW, screenH);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(left, right, bottom, top, -100, 100); /* left, right, bottom, top, near, far */

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   glClear(GL_COLOR_BUFFER_BIT);

   glTranslated((right - left) / 2, (bottom - top) / 2, 0);
   glScaled(zoom, zoom, zoom);
   glTranslated((left - right) / 2, (top - bottom) / 2, 0);

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
         glColor4d(uiPlayer[p].color.r, uiPlayer[p].color.g, uiPlayer[p].color.b, bright);
         glBegin(GL_LINE_STRIP);
         for(i = 0; i < shot->length; ++i)
         {
            glVertex2d(shot->dot[i].x, shot->dot[i].y);
         }
         glEnd();
      }
      glColor3d(uiPlayer[p].color.r, uiPlayer[p].color.g, uiPlayer[p].color.b);
      glPushMatrix();
      glTranslated(pl->position.x, pl->position.y, 0);
      glutWireSphere(4.0, 16, 2);
      glPopMatrix();
   }

   glColor3d(0.3, 0.3, 0.3);
   for(i = 0; i < conf.numPlanets; ++i)
   {
      SimPlanet* p = getPlanet(i);
      glPushMatrix();
      glTranslated(p->position.x, p->position.y, 0);
      glutWireSphere(p->radius, 16, 2);
      glPopMatrix();
   }

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, screenW, screenH, 0, -100, 100); /* left, right, bottom, top, near, far */

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   if(fps) drawFps();
   drawPlayers();

   glFlush();
   glutSwapBuffers();
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

void initInterface(int* argc, char** argv)
{
   int p;

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

   glutDisplayFunc(display);
   glutReshapeFunc(reshape);

   glEnable(GL_MULTISAMPLE);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

void stepInterface(void)
{
   glutPostRedisplay();
   glutMainLoopEvent();
}

void updateZoom(double z)
{
   zoom = LIMIT(z, 0.0, 1.0);
}

void toggleFps(void)
{
   fps = !fps;
}
