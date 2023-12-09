#include "network.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
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

#include "config.h"
#include "simulation.h"
#include "display.h"

#ifdef _WIN32
#define close closesocket
#ifndef IPV6_V6ONLY
#define IPV6_V6ONLY 27
#if UINT_MAX == 0xfffffffful
typedef unsigned int uint32_t;
#else
#error "4 byte unsigned int unknown"
#endif
#endif
#else
#endif

#define PORT "3490"
#define BACKLOG 4
#define WELCOME "\r\nUse \"n name\" to change name, \"v velocity\" to change velocity, \"c\" to clear past shots or \"q\" to close the connection.\r\nEverything else is interpreted as a shooting angle.\r\n\r\n> "

typedef struct
{
   int socket;
   char msgbuf[128];
   int msgbufindex;
   int echo;
   int bot;
   int local;
   int limit;
   int controller;
   char ip[INET6_ADDRSTRLEN];
   int timeout;
} connection_t;

typedef struct
{
   char ip[INET6_ADDRSTRLEN];
   int time;
} block_entry_t;

static fd_set master, readfds;
static int fdmax, listener;
static char buf[128];
static char sendbuf[128];
static uint32_t binsend[5000];
static block_entry_t block_list[16];
static connection_t* connection;

static void print_error(const char* msg)
{
   #if _WIN32
   LPVOID lpMsgBuf;
   FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      GetLastError(),
      MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
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

static void snd(int socket, char* msg)
{
   int flags;
   #if defined __APPLE__ || defined _WIN32
   flags = 0;
   #else
   flags = MSG_NOSIGNAL | MSG_DONTWAIT;
   #endif
   if(send(socket, msg, strlen(msg), flags) == -1)
   {
      print_error("send");
   }
}

static void snd_l(int socket, int len, uint32_t* msg)
{
   int flags;
   #if defined __APPLE__ || defined _WIN32
   flags = 0;
   #else
   flags = MSG_NOSIGNAL | MSG_DONTWAIT;
   #endif
   if(send(socket, (char*)msg, sizeof(uint32_t) * len, flags) == -1)
   {
      print_error("send");
   }
}

static int is_blocked(char* ip)
{
   int entry_min_time = -1;
   int min_time = INT_MAX;
   int i;

   for(i = 0; i < 16; ++i)
   {
      if(strcmp(ip, block_list[i].ip) == 0)
      {
         int time = block_list[i].time;
         block_list[i].time = 600;
         return time;
      }
      if(block_list[i].time < min_time)
      {
         min_time = block_list[i].time;
         entry_min_time = i;
      }
   }
   strncpy(block_list[entry_min_time].ip, ip, INET6_ADDRSTRLEN);
   block_list[entry_min_time].time = 600;
   return 0;
}

static int is_connected(char* ip)
{
   int k;
   for(k = 0; k < conf.maxPlayers; ++k)
   {
      if(connection[k].socket)
      {
         if(strcmp(ip, connection[k].ip) == 0)
         {
            return 1;
         }
      }
   }
   return 0;
}

static void update_block_list()
{
   int i;

   for(i = 0; i < 16; ++i)
   {
      if(block_list[i].time)
      {
         block_list[i].time--;
         if(block_list[i].time == 0)
         {
            block_list[i].ip[0] = '\0';
         }
      }
   }
}

static void update_limits()
{
   static int counter;
   int k;

   counter++;
   if(counter % 3 != 0) return;

   for(k = 0; k < conf.maxPlayers; ++k)
   {
      if(connection[k].socket)
      {
         connection[k].limit += 1;
         if(connection[k].limit > 512)
         {
            connection[k].limit = 512;
         }
      }
   }
}

static void update_timeouts()
{
   int k;

   for(k = 0; k < conf.maxPlayers; ++k)
   {
      if(connection[k].socket)
      {
         if(getMode() == MODE_PLAYING && connection[k].timeout)
         {
            connection[k].timeout--;
         }
      }
   }
}

void initNetwork(void)
{
   struct addrinfo hints, *ai, *p;
   int yes = 1;
   int no = 0;
   int rv;

   #ifdef _WIN32
   WSADATA wsaData;
   if(WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
   {
      fprintf(stderr, "WSAStartup failed.\n");
      exit(1);
   }
   #endif

   connection = malloc(conf.maxPlayers * sizeof(connection_t));
   memset(connection, 0, conf.maxPlayers * sizeof(connection_t));

   FD_ZERO(&master);
   FD_ZERO(&readfds);

   memset(&hints, 0, sizeof hints);
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE;
   if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0)
   {
      fprintf(stderr, "getaddrinfo: %s", gai_strerror(rv));
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

      if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, (void *)&yes, sizeof(yes)) == -1)
      {
         print_error("setsockopt reuse");
         close(listener);
         continue;
      }

      // only interested if dualstack
      if (setsockopt(listener, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&no, sizeof(no)) == -1)
      {
         print_error("setsockopt dualstack");
         close(listener);
         continue;
      }

      if (bind(listener, p->ai_addr, p->ai_addrlen) == -1)
      {
         print_error("bind");
         close(listener);
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

         if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, (void *)&yes, sizeof(yes)) == -1)
         {
            print_error("setsockopt reuse");
            close(listener);
            continue;
         }

         if (bind(listener, p->ai_addr, p->ai_addrlen) == -1)
         {
            print_error("bind");
            close(listener);
            continue;
         }

         break; // success
      }

      if (p == NULL) // IPv4 failed as well
      {
         fprintf(stderr, "failed to bind\n");
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
   printf("waiting for connections...\n");
}

void disconnectPlayer(int p)
{
   int socket = connection[p].socket;
   connection[p].echo = 0;
   playerLeave(p);
   close(connection[p].socket);
   FD_CLR(connection[p].socket, &master);
   connection[p].socket = 0;
   connection[p].bot = 0;
   printf("socket %d closed\n", socket);
}

void sendOwnId(int i, int p)
{
   binsend[0] = MSG_OWNID;
   binsend[1] = p;
   snd_l(i, 2, binsend);
}

void sendPlayerLeave(int i, int p)
{
   binsend[0] = MSG_PLAYERLEAVE;
   binsend[1] = p;
   snd_l(i, 2, binsend);
}

void sendPlayerPos(int i, int p)
{
   float tmp;
   binsend[0] = MSG_PLAYERPOS;
   binsend[1] = p;
   tmp = getPlayer(p)->position.x;
   memcpy(&binsend[2], &tmp, sizeof(float));
   tmp = getPlayer(p)->position.y;
   memcpy(&binsend[3], &tmp, sizeof(float));
   snd_l(i, 4, binsend);
}

void sendPlanets(int i)
{
   if (conf.sendPlanets == 0)
   {
      return;
   }
    if (connection[i].socket && connection[i].bot)
    {
        int planet_size = sizeof(SimPlanet) / sizeof(uint32_t);
        binsend[0] = MSG_PLANETPOS;
        binsend[1] = conf.numPlanets;
        binsend[2] = sizeof(SimPlanet);
        int buf_index = 3;
        for (int planet_index = 0; planet_index < conf.numPlanets; planet_index++, buf_index += planet_size)
        {
            SimPlanet *temp = getPlanet(planet_index);
            memcpy(&binsend[buf_index], &temp->position.x, planet_size);
            memcpy(&binsend[buf_index+2], &temp->position.y, planet_size);
            memcpy(&binsend[buf_index+4], &temp->radius, planet_size);
            memcpy(&binsend[buf_index+6], &temp->mass, planet_size);
        }
        snd_l(connection[i].socket, buf_index, binsend);
    }
}

void sendShotFinished(int i, SimShot* s)
{
   binsend[0] = MSG_SHOTFINISHED;
   binsend[1] = s->player;
   binsend[2] = s->length;
   memcpy(&binsend[3], s->dot, s->length * 2 * sizeof(float));
   snd_l(i, 3 + s->length * 2, binsend);
}

void sendShotBegin(int i, SimShot* s)
{
   double tmp;
   binsend[0] = MSG_SHOTBEGIN;
   binsend[1] = s->player;
   tmp = s->angle;
   memcpy(&binsend[2], &tmp, sizeof(double));
   tmp = s->velocity;
   memcpy(&binsend[4], &tmp, sizeof(double));
   snd_l(i, 6, binsend);
}

void sendShotFin(int i, SimShot* s)
{
   double tmp;
   binsend[0] = MSG_SHOTFIN;
   binsend[1] = s->player;
   tmp = s->angle;
   memcpy(&binsend[2], &tmp, sizeof(double));
   tmp = s->velocity;
   memcpy(&binsend[4], &tmp, sizeof(double));
   binsend[6] = s->length;
   memcpy(&binsend[7], s->dot, s->length * 2 * sizeof(float));
   snd_l(i, 7 + s->length * 2, binsend);
}

void sendOwnEnergy(int i, int p)
{
   double tmp;
   binsend[0] = MSG_OWN_ENERGY;
   binsend[1] = p;
   tmp = getPlayer(p)->energy;
   memcpy(&binsend[2], &tmp, sizeof(double));
   snd_l(i, 4, binsend);
}

void sendGameMode(int i, int p)
{
   binsend[0] = MSG_GAMEMODE;
   binsend[1] = p;
   binsend[2] = MODE_REALTIME;
   snd_l(i, 3, binsend);
}

void allSendPlayerLeave(int p)
{
   int k;
   for(k = 0; k < conf.maxPlayers; ++k)
   {
      if(connection[k].socket && connection[k].bot)
      {
         sendPlayerLeave(connection[k].socket, p);
      }
   }
}

void allSendPlayerPos(int p)
{
   int k;
   for(k = 0; k < conf.maxPlayers; ++k)
   {
      if(connection[k].socket && connection[k].bot)
      {
         sendPlayerPos(connection[k].socket, p);
      }
   }
}

void allSendShotFinished(SimShot* s)
{
   int k;
   for(k = 0; k < conf.maxPlayers; ++k)
   {
      if(connection[k].socket && connection[k].bot)
      {
         if(connection[k].bot >= MSG_SHOTFIN)
         {
            sendShotFin(connection[k].socket, s);
         }
         else
         {
            sendShotFinished(connection[k].socket, s);
         }
      }
   }
}

void allSendShotBegin(SimShot* s)
{
   int k;
   for(k = 0; k < conf.maxPlayers; ++k)
   {
      if(connection[k].socket && connection[k].bot)
      {
         if(connection[k].bot >= MSG_SHOTBEGIN)
         {
            sendShotBegin(connection[k].socket, s);
         }
      }
   }
}

void allSendKillMessage(int p, int p2)
{
   int k;
   sprintf(sendbuf, "%s killed %s", getPlayer(p)->name, getPlayer(p2)->name);
   for(k = 0; k < conf.maxPlayers; ++k)
   {
      if(connection[k].socket && !connection[k].bot && !connection[k].controller)
      {
         snd(connection[k].socket, "\r\n");
         snd(connection[k].socket, sendbuf);
         snd(connection[k].socket, "\r\n> ");
      }
   }
}

void stepNetwork(void)
{
   int i, k, pi, pi2, nbytes, newfd;
   char remoteIP[INET6_ADDRSTRLEN];
   struct sockaddr_storage remoteaddr;
   socklen_t addrlen;
   struct timeval tv;

   update_block_list();
   update_limits();
   update_timeouts();

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
               int blocked, connected, local;
               getnameinfo((struct sockaddr *)&remoteaddr, addrlen, remoteIP, sizeof remoteIP, NULL, 0, NI_NUMERICHOST | NI_NUMERICSERV);
               local =   (  (strcmp(remoteIP,"127.0.0.1") == 0)
                         || (strcmp(remoteIP,"::ffff:127.0.0.1") == 0)
                         || (strcmp(remoteIP,"::1") == 0)
                         );
               blocked = local ? 0 : is_blocked(remoteIP);
               connected = local ? 0 : is_connected(remoteIP);
               if(blocked)
               {
                  close(newfd);
                  printf("new connection from %s on socket %d refused: blocked for %lfs\n", remoteIP, newfd, blocked / 60.0f);
               }
               else if(connected)
               {
                  close(newfd);
                  printf("new connection from %s on socket %d refused: already connected\n", remoteIP, newfd);
               }
               else
               {
                  for(k = 0; k < conf.maxPlayers; ++k)
                  {
                     if(connection[k].socket == 0)
                     {
                        connection[k].socket = newfd;
                        connection[k].local = local;
                        connection[k].limit = 512;
                        connection[k].timeout = 5 * 60 * 60;
                        strncpy(connection[k].ip, remoteIP, INET6_ADDRSTRLEN);
                        playerJoin(k);
                        updateName(k, "Anonymous");
                        allSendPlayerPos(k);
                        FD_SET(newfd, &master);
                        if(newfd > fdmax)
                        {
                           fdmax = newfd;
                        }
                        printf("new connection from %s on socket %d accepted\n", remoteIP, newfd);
                        snd(newfd, WELCOME);
                        break;
                     }
                  }
                  if(k == conf.maxPlayers)
                  {
                     close(newfd);
                     printf("new connection from %s on socket %d refused: max connections\n", remoteIP, newfd);
                  }
               }
            }
         }
         else
         {
            pi = -1;
            for(k = 0; k < conf.maxPlayers; ++k)
            {
               if(connection[k].socket == i)
               {
                  pi = k;
                  break;
               }
            }
            nbytes = recv(i, buf, sizeof buf, 0);
            connection[pi].limit -= connection[pi].local ? 0 : nbytes;
            if (  (nbytes <= 0)
               || (connection[pi].limit < 0)
               )
            {
               if(connection[pi].limit < 0)
               {
                  printf("socket %d exceeded rate limit\n", i);
               }
               else if(nbytes == 0)
               {
                  printf("socket %d hung up\n", i);
               }
               else
               {
                  print_error("recv");
               }
               disconnectPlayer(pi);
               allSendPlayerLeave(pi);
               close(i);
               FD_CLR(i, &master);
            }
            else
            {
               connection[pi].timeout = 5 * 60 * 60;
               for(k = 0; k < nbytes && pi >= 0; ++k)
               {
                  unsigned char c = buf[k];
                  if(c != '\r' && c != '\n')
                  {
                     if(isprint(c) && connection[pi].msgbufindex < 128 - 2)
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
                     if(connection[pi].echo)
                     {
                        snd(i, connection[pi].msgbuf);
                        snd(i, "\r\n");
                     }
                     if(!conf.fastmode)
                     {
                        printf("%16s (%d): \"%s\"\n", getPlayer(pi)->name, pi, connection[pi].msgbuf);
                     }
                     switch(connection[pi].msgbuf[0])
                     {
                        case 'n':
                        {
                           updateName(pi, connection[pi].msgbuf + 2);
                           break;
                        }
                        case 't':
                        {
                           if(connection[pi].local)
                           {
                              tankEnergy(atoi(connection[pi].msgbuf + 2));
                           }
                          break;
                        }
                        case 'a':
                        {
                           if(connection[pi].local)
                           {
                              conf.area = !conf.area;
                           }
                          break;
                        }
                        case 'p':
                        {
                           if(connection[pi].local)
                           {
                              conf.pot = !conf.pot;
                           }
                          break;
                        }
                        case 'v':
                        {
                           updateVelocity(pi, atof(connection[pi].msgbuf + 2));
                           break;
                        }
                        case 'z':
                        {
                           if(connection[pi].local)
                           {
                              updateZoom(atof(connection[pi].msgbuf + 2));
                           }
                           break;
                        }
                        case 'T':
                        {
                           if(connection[pi].local)
                           {
                              conf.throttle = atoi(connection[pi].msgbuf + 2);
                           }
                           break;
                        }
                        case 'D':
                        {
                           if(connection[pi].local)
                           {
                              conf.debug = !conf.debug;
                           }
                           break;
                        }
                        case 'F':
                        {
                           if(connection[pi].local)
                           {
                              conf.fastmode = !conf.fastmode;
                           }
                           break;
                        }
                        case 'c':
                        {
                           clearTraces(pi);
                           break;
                        }
                        case 'b':
                        {
                           if(connection[pi].bot)
                           {
                              connection[pi].bot = 0;
                           }
                           else
                           {
                              connection[pi].bot = atoi(connection[pi].msgbuf + 2);
                              if(connection[pi].bot == 0)
                              {
                                 connection[pi].bot = 4;
                              }
                              if(connection[pi].bot >= MSG_GAMEMODE && connection[pi].bot <= MSG_OWN_ENERGY)
                              {
                                 sendGameMode(i, pi);
                              }
                              sendOwnId(i, pi);
                              sendPlanets(pi);
                              for(pi2 = 0; pi2 < conf.maxPlayers; ++pi2)
                              {
                                 if(connection[pi2].socket)
                                 {
                                    sendPlayerPos(i, pi2);
                                 }
                              }
                           }
                           break;
                        }
                        case 'd':
                        {
                           if(connection[pi].local)
                           {
                              connection[pi].controller = atoi(connection[pi].msgbuf + 2);
                           }
                           break;
                        }
                        case 'f':
                        {
                           if(connection[pi].local)
                           {
                              toggleFps();
                           }
                           break;
                        }
                        case 'i':
                        {
                           if(connection[pi].local)
                           {
                              reinitialize();
                           }
                           break;
                        }
                        case 'x':
                        {
                           if(connection[pi].local)
                           {
                              exit(0);
                           }
                           break;
                        }
                        case 'e':
                        {
                           connection[pi].echo = !connection[pi].echo;
                           break;
                        }
                        case 'q':
                        {
                           disconnectPlayer(pi);
                           allSendPlayerLeave(pi);
                           break;
                        }
                        case 'u':
                        {
                           if(connection[pi].bot >= MSG_OWN_ENERGY)
                           {
                              sendOwnEnergy(i, pi);
                           }
                           else if(!connection[pi].bot)
                           {
                              sprintf(sendbuf,"%4d\n", (int)getPlayer(pi)->energy);
                              snd(i, sendbuf);
                           }
                           break;
                        }
                        case 'g':
                        {
                           rgb col = getColor(pi);
                           sprintf(sendbuf,"%3.0lf %3.0lf %3.0lf\n", col.r * 255, col.g * 255, col.b * 255);
                           snd(i, sendbuf);
                           break;
                        }
                        default:
                        {
                           double angle = atof(connection[pi].msgbuf);
                           if(!connection[pi].bot)
                           {
                              angle = -angle + 90.0; // convert from 0° top clockwise to mathematical (0° right ccw)
                           }
                           updateAngle(pi, angle, connection[pi].controller);
                           break;
                        }
                     }

                     if(connection[pi].socket && !connection[pi].bot && !connection[pi].controller)
                     {
                        snd(i, "> ");
                     }
                  }
               }
            }
         }
      }
   }
   for(k = 0; k < conf.maxPlayers; ++k)
   {
      if(connection[k].socket && connection[k].timeout == 0)
      {
         disconnectPlayer(k);
         allSendPlayerLeave(k);
      }
   }   
}
