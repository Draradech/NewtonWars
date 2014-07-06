#include "simulation.h"

#include <stdio.h>
#include <math.h>

#include <GL/freeglut.h>

#include "config.h"
#include "color.h"

#define DOTS 3000

typedef struct
{
   int exponent;
   int which;
   rgb color;
   Vec2d dot[DOTS];
   Vec2d currentShot[MAXSEGMENTS];
   int dotIndex;
   int csIndex;
   int csIndexTakeover;
   SimPlayer p;
   char nick[16];
   int valid;
} UiPlayer;

UiPlayer uiPlayer[MAXPLAYERS];

char sbuf[256];

int fps, confirm, uiCurrentPlayer;
int scores = 1;

double left, right, bottom, top, zoom;
int screenW, screenH;

void drawFps(void)
{
  static int frameCounter;
  static char buffer[64];
  static unsigned long timeOld;
  unsigned long time;
  static double dfps;

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
  glRasterPos2d(3.0, 54.0);
  glutBitmapString(GLUT_BITMAP_9_BY_15, (unsigned char*)buffer);
}

void drawPlayers(void)
{
  static char buffer[128];
  int p;

  for(p = 0; p < MAXPLAYERS; ++p)
  {
    if(!uiPlayer[p].p.active) continue;
    glColor3d(uiPlayer[p].color.r, uiPlayer[p].color.g, uiPlayer[p].color.b);

    sprintf(buffer, "%s%s (%d:%d)%s",
      p == uiCurrentPlayer ? "> " : "  ",uiPlayer[p].nick, uiPlayer[p].p.kills, uiPlayer[p].p.deaths, p == uiCurrentPlayer ? " <" : "");
    //glRasterPos2d((p / 2) * (screenW / ((MAXPLAYERS + 1) / 2)) + 3.0, (p % 2) ? screenH - 6.0 : 15.0);
    glRasterPos2d(p * screenW / MAXPLAYERS + 3.0, 15.0);
    glutBitmapString(GLUT_BITMAP_9_BY_15, (unsigned char*)buffer);
  }
}

void display(void)
{
  int i, p;

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

  for(p = 0; p < MAXPLAYERS; ++p)
  {
    if(!uiPlayer[p].p.active) continue;
    glBegin(GL_LINE_STRIP);
    glColor3d(1.0, 1.0, 1.0);
    #if FAST
    while(uiPlayer[p].csIndexTakeover < uiPlayer[p].csIndex)
    {
      uiPlayer[p].dot[uiPlayer[p].dotIndex] = uiPlayer[p].currentShot[uiPlayer[p].csIndexTakeover];
      uiPlayer[p].dotIndex = (uiPlayer[p].dotIndex + 1) % DOTS;
      uiPlayer[p].csIndexTakeover++;
    }
    uiPlayer[p].csIndexTakeover = uiPlayer[p].csIndex = 0;
    #else
    if(uiPlayer[p].csIndexTakeover < uiPlayer[p].csIndex)
    {
      uiPlayer[p].dot[uiPlayer[p].dotIndex] = uiPlayer[p].currentShot[uiPlayer[p].csIndexTakeover];
      uiPlayer[p].dotIndex = (uiPlayer[p].dotIndex + 1) % DOTS;
      uiPlayer[p].csIndexTakeover++;
    }
    else
    {
      uiPlayer[p].csIndexTakeover = uiPlayer[p].csIndex = 0;
    }
    #endif
    for(i = 0; i < DOTS; ++i)
    {
      int j = (uiPlayer[p].dotIndex + i) % DOTS;
      int jo = (uiPlayer[p].dotIndex + i - 1) % DOTS;
      double bright = (double)i / DOTS;
      bright *= bright;
      glColor4d(uiPlayer[p].color.r, uiPlayer[p].color.g, uiPlayer[p].color.b, bright);
      if(distance(uiPlayer[p].dot[j], uiPlayer[p].dot[jo]) > 30.0)
      {
        glEnd();
        glBegin(GL_LINE_STRIP);
      }
      glVertex2d(uiPlayer[p].dot[j].x, uiPlayer[p].dot[j].y);
    }
    glEnd();

    glColor3d(uiPlayer[p].color.r, uiPlayer[p].color.g, uiPlayer[p].color.b);
    glPushMatrix();
    glTranslated(uiPlayer[p].p.position.x, uiPlayer[p].p.position.y, 0);
    glutWireSphere(4.0, 16, 2);
    glPopMatrix();
  }

  glColor3d(0.3, 0.3, 0.3);
  for(i = 0; i < PLANETS; ++i)
  {
    glPushMatrix();
    glTranslated(planet[i].position.x, planet[i].position.y, 0);
    glutWireSphere(planet[i].radius, 16, 2);
    glPopMatrix();
  }

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, screenW, screenH, 0, -100, 100); /* left, right, bottom, top, near, far */

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  if(scores) drawPlayers();
  if(fps) drawFps();

  glFlush();
  glutSwapBuffers();
}

void clearDots(void)
{
  int i, p;
  Vec2d d = {-10.0, -10.0};

  for(p = 0; p < MAXPLAYERS; ++p)
  {
    if(!uiPlayer[p].p.active) continue;
    for(i = 0; i < DOTS; ++i)
    {
      uiPlayer[p].dot[i] = d;
    }
  }
}

void clearDotsP(int p)
{
  int i;
  Vec2d d = {-10.0, -10.0};

  for(i = 0; i < DOTS; ++i)
  {
     uiPlayer[p].dot[i] = d;
  }
}

void initUiPlayer(int p)
{
  hsv color;
  uiPlayer[p].exponent = 0;
  color.h = 360.0 / MAXPLAYERS * p;
  color.s = 0.8;
  color.v = 1.0;
  uiPlayer[p].color = hsv2rgb(color);
  uiPlayer[p].dotIndex = 0;
  uiPlayer[p].csIndex = 0;
  uiPlayer[p].csIndexTakeover = 0;
  sprintf(uiPlayer[p].nick, "Player %d", p);
  clearDotsP(p);
  initPlayer(p, 1);
}

void uiInit(void)
{
  int p;
  for(p = 0; p < MAXPLAYERS; ++p)
  {
    if(!uiPlayer[p].p.active) continue;
    initUiPlayer(p);
  }
  clearDots();
  zoom = 0.8;
}

void idle(void)
{
   int p, p2, ready = 1;

   for(p = 0; p < MAXPLAYERS; ++p)
   {
      if(!uiPlayer[p].p.active) continue;
      if(uiPlayer[p].csIndex)
      {
         ready = 0;
      }
   }

   if(ready)
   {
      for(p = 0; p < MAXPLAYERS; ++p)
      {
         if(player[p].deaths > uiPlayer[p].p.deaths)
         {
            clearDotsP(p);
            sprintf(sbuf, "\r\n%s killed %s\r\n> ", uiPlayer[uiCurrentPlayer].nick, uiPlayer[p].nick);
            for(p2 = 0; p2 < MAXPLAYERS; ++p2)
            {
               uiPlayer[p2].valid = 0;
               player[p2].power = 10.0;
               if(player[p2].active && send(connection[p2].socket, sbuf, strlen(sbuf), MSG_NOSIGNAL) == -1)
               {
                  print_error("send");
               }
            }
         }
         uiPlayer[p].p = player[p];
      }
      uiCurrentPlayer = currentPlayer;
      if(uiPlayer[currentPlayer].valid)
      {
         fireBullet();
         uiPlayer[currentPlayer].valid = 0;
         player[currentPlayer].power = 10.0;
         nextPlayer();
      }
   }

   glutPostRedisplay();
}

void registerDot(void)
{
   uiPlayer[currentPlayer].currentShot[uiPlayer[currentPlayer].csIndex] = bullet.position;
   uiPlayer[currentPlayer].csIndex++;
}

void keyboard(unsigned char key, int x, int y)
{
   switch(key)
   {
      case 27:
      {
         if(confirm) glutLeaveMainLoop();
         break;
      }
      case 32:
      {
         if(confirm) uiInit();
         break;
      }
      case 'c':
      {
         confirm = 1;
         return;
      }
      case 'f':
      {
         fps = !fps;
         break;
      }
      case 's':
      {
         scores = !scores;
         break;
      }
   }
   confirm = 0;
}

void mouse(int button, int state, int x, int y)
{
}

void reshape(int w, int h)
{
   double ratioScreen = (double)w / h;
   double ratioSim = W / H;
   screenW = w;
   screenH = h;
   if(ratioScreen > ratioSim)
   {
      top = 0.0;
      bottom = H;
      left = -(H * ratioScreen - W) / 2.0;
      right = W + (H * ratioScreen - W) / 2.0;
   }
   else
   {
      top = -(W / ratioScreen - H) / 2.0;
      bottom = H + (W / ratioScreen - H) / 2.0;
      left = 0.0;
      right = W;
   }
}

void initInterface(int* argc, char** argv)
{
    glutInit(argc, argv);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);

    glutCreateWindow("Newton Wars");
    #if FULLSCREEN
    glutFullScreen();
    #else
    glutPositionWindow(0, 0);
    glutReshapeWindow(800, 600);
    #endif

    glutDisplayFunc(display);
    glutIdleFunc(idle);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    setTraceCallback(&registerDot);

    uiInit();
}

void stepInterface(void)
{
   glutMainLoopEvent();
}

