#include <stdio.h>
#include <time.h>
#include <math.h>

#include <GL/freeglut.h>

#include "color.h"
#include "simulation.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

#ifdef _WIN32
#define close closesocket
#ifndef IPV6_V6ONLY
#define IPV6_V6ONLY 27
#endif
#else
#endif

#define PORT "3490"
#define BACKLOG 4
#define NUMCONNS MAXPLAYERS
#define WELCOME "\xff\xfc\x01\r\nUse \"n name\" to change name, \"p power\" to change power or \"c\" to clear past shots.\r\nEverything else is interpreted as a shooting angle.\r\n\r\n> "

#define DOTS 3000
#define LIMIT(x, min, max) (((x) < (min)) ? (min) : ((x) > (max)) ? (max) : (x))
#define FAST 0
#define FULLSCREEN 1

typedef struct
{
   int socket;
   char msgbuf[128];
   int msgbufindex;
} connection_t;

connection_t connection[NUMCONNS];

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

void initNet(void);
void netSelect(void);

void print_error(const char* msg)
{
   #if _WIN32
   LPVOID lpMsgBuf;
   FormatMessage( 
       FORMAT_MESSAGE_ALLOCATE_BUFFER | 
       FORMAT_MESSAGE_FROM_SYSTEM | 
       FORMAT_MESSAGE_IGNORE_INSERTS,
       NULL,
       GetLastError(),
       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
       (LPTSTR) &lpMsgBuf,
       0,
       NULL 
   );
   fprintf(stderr, "%s: %s", msg, (LPCTSTR)lpMsgBuf);
   LocalFree( lpMsgBuf );
   #else
   perror(msg);
   #endif
}

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
  init();
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

   netSelect();
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

int main(int argc, char** argv)
{
    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);

    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);

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
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);

    setTraceCallback(&registerDot);

    uiInit();
    initNet();

    glutMainLoop();

    return 0;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
   if (sa->sa_family == AF_INET)
   {
      return &(((struct sockaddr_in*)sa)->sin_addr);
   }

   return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

fd_set master, readfds;
int fdmax, listener;
char buf[256];
char sendbuf[256];

void initNet(void)
{
   struct addrinfo hints, *ai, *p;
   int yes = 1;
   int no = 0;
   int rv;
   
   #ifdef _WIN32
   WSADATA wsaData;
   if(WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
   {
      fprintf(stderr, "WSAStartup failed.\r\n");
      exit(1);
   }
   #endif

   FD_ZERO(&master);
   FD_ZERO(&readfds);

   memset(&hints, 0, sizeof hints);
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE;
   if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0)
   {
      fprintf(stderr, "getaddrinfo: %s\r\n", gai_strerror(rv));
      exit(2);
   }

   // loop through all the results and bind to the first IPv6 we can
   for(p = ai; p != NULL; p = p->ai_next)
   {
      if(p->ai_family != AF_INET6)
      {
         continue;
      }

      if ((listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
      {
         print_error("socket");
         continue;
      }

      if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
      {
         print_error("setsockopt reuse");
         continue;
      }
   
      // only interested if dualstack
      if (setsockopt(listener, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(no)) == -1)
      {
         print_error("setsockopt dualstack");
         continue;
      }

      if (bind(listener, p->ai_addr, p->ai_addrlen) == -1)
      {
         close(listener);
         print_error("bind");
         continue;
      }

      break; // success
   }

   if (p == NULL) // IPv6 dualstack failed
   {
      // loop through all the results and bind to the first IPv4 we can
      for(p = ai; p != NULL; p = p->ai_next)
      {
         if(p->ai_family != AF_INET)
         {
            continue;
         }

         if ((listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
         {
            print_error("socket");
            continue;
         }

         if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
         {
            print_error("setsockopt");
            continue;
         }

         if (bind(listener, p->ai_addr, p->ai_addrlen) == -1)
         {
            close(listener);
            print_error("bind");
            continue;
         }

         break; // success
      }

      if (p == NULL) // IPv4 failed as well
      {
         fprintf(stderr, "failed to bind\r\n");
         exit(3);
      }
   }

   freeaddrinfo(ai);

   if (listen(listener, BACKLOG) == -1)
   {
      print_error("listen");
      exit(4);
   }

   FD_SET(listener, &master);
   fdmax = listener;
   printf("waiting for connections...\r\n");
}

void netSelect(void)
{
   int i, k, pi, nbytes, newfd;
   char remoteIP[INET6_ADDRSTRLEN];
   struct sockaddr_storage remoteaddr;
   socklen_t addrlen;
   struct timeval tv;
   
   tv.tv_sec = 0;
   tv.tv_usec = 1;
   readfds = master;
   if(select(fdmax + 1, &readfds, NULL, NULL, &tv) == -1)
   {
      print_error("select");
      exit(5);
   }
   
   for(i = 0; i <= fdmax; ++i)
   {
      if(FD_ISSET(i, &readfds))
      {
         if(i == listener)
         {
            addrlen = sizeof remoteaddr;
            newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);
            if(newfd == -1)
            {
               print_error("accept");
            }
            else
            {
               getnameinfo((struct sockaddr *)&remoteaddr, addrlen, remoteIP, sizeof remoteIP, NULL, 0, NI_NUMERICHOST | NI_NUMERICSERV);
               for(k = 0; k < NUMCONNS; ++k)
               {
                  if(connection[k].socket == 0)
                  {
                     connection[k].socket = newfd;
                     player[k].active = 1;
                     initUiPlayer(k);
                     if(k == 0)
                     {
                        for(pi = 1; pi < MAXPLAYERS; pi++)
                        {
                           if(player[pi].active)
                           {
                              break;
                           }
                        }
                        if(pi == MAXPLAYERS)
                        {
                           currentPlayer = 0;
                        }
                     }
                     break;
                  }
               }
               if(k == NUMCONNS)
               {
                  close(newfd);
                  printf("new connection from %s on socket %d refused: max connections\r\n", remoteIP, newfd);
               }
               else
               {
                  FD_SET(newfd, &master);
                  if(newfd > fdmax)
                  {
                     fdmax = newfd;
                  }
                  printf("new connection from %s on socket %d accepted\r\n", remoteIP, newfd);
                  if(send(newfd, WELCOME, strlen(WELCOME), MSG_NOSIGNAL) == -1)
                  {
                     print_error("send");
                  }
               }
            }
         }
         else
         {
            if((nbytes = recv(i, buf, sizeof buf, 0)) <= 0)
            {
               if(nbytes == 0)
               {
                  printf("socket %d hung up\r\n", i);
               }
               else
               {
                  print_error("recv");
               }
               for(k = 0; k < NUMCONNS; ++k)
               {
                  if(connection[k].socket == i)
                  {
                     connection[k].socket = 0;
                     player[k].active = 0;
                     uiPlayer[k].nick[0] = '\0';
                     if(k == currentPlayer)
                     {
                        nextPlayer();
                     }
                     break;
                  }
               }   
               close(i);
               FD_CLR(i, &master);
            }
            else
            {
               pi = -1;
               for(k = 0; k < NUMCONNS; ++k)
               {
                  if(connection[k].socket == i)
                  {
                     pi = k;
                     break;
                  }
               }   
               for(k = 0; k < nbytes && pi >= 0; ++k)
               {
                  unsigned char c = buf[k];
                  //printf("%x ", c);
                  if(c != '\r' && c != '\n' && c != 0)
                  {
                     if(connection[pi].msgbufindex < 128 - 2)
                     {
                        connection[pi].msgbuf[connection[pi].msgbufindex++] = c;
                     }
                  }
                  else
                  {
                     if(connection[pi].msgbufindex == 0)
                     {
                        continue;
                     }
                     connection[pi].msgbuf[connection[pi].msgbufindex] = '\0';
                     connection[pi].msgbuf[connection[pi].msgbufindex + 1] = '\0';
                     connection[pi].msgbufindex = 0;
                     if(send(i, connection[pi].msgbuf, strlen(connection[pi].msgbuf), MSG_NOSIGNAL) == -1)
                     {
                        print_error("send");
                     }
                     if(send(i, "\r\n", 2, MSG_NOSIGNAL) == -1)
                     {
                        print_error("send");
                     }
                     printf("%s: \"%s\"\r\n", uiPlayer[pi].nick, connection[pi].msgbuf);
                     switch(connection[pi].msgbuf[0])
                     {
                        case 'n':
                        {
                           snprintf(uiPlayer[pi].nick, 16, "%s", connection[pi].msgbuf + 2);
                           printf("Nick parsed as: %s\r\n\r\n", uiPlayer[pi].nick);
                           break;
                        }
                        case 'p':
                        {
                           double d;
                           
                           d = atof(connection[pi].msgbuf + 2);
                           printf("Power parsed as: %.20lf\r\n\r\n", d);
                           player[pi].power = LIMIT(d, 0.0, 15.0);
                           sprintf(sendbuf, "Power set to %lf for next shot\r\n", player[pi].power);
                           if(send(i, sendbuf, strlen(sendbuf), MSG_NOSIGNAL) == -1)
                           {
                              print_error("send");
                           }
                           break;
                        }
                        case 'z':
                        {
                           double d;
                           
                           d = atof(connection[pi].msgbuf + 2);
                           printf("Zoom parsed as: %.20lf\r\n\r\n", d);
                           zoom = LIMIT(d, 0.0, 1.0);
                           sprintf(sendbuf, "Zoom set to %lf\r\n", zoom);
                           if(send(i, sendbuf, strlen(sendbuf), MSG_NOSIGNAL) == -1)
                           {
                              print_error("send");
                           }
                           break;
                        }
                        case 'c':
                        {
                           clearDotsP(pi);
                           printf("Parsed as clear.\r\n\r\n");
                           break;
                        }
                        default:
                        {
                           double d;
                           
                           d = atof(connection[pi].msgbuf);
                           printf("Angle parsed as: %.20lf\r\n\r\n", d);
                           player[pi].angle = d;
                           uiPlayer[pi].valid = 1;
                           break;
                        }
                     }

                     if(send(i, "> ", 2, MSG_NOSIGNAL) == -1)
                     {
                        print_error("send");
                     }
                  }
               }
            }
         }
      }
   }
}
