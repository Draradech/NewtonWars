#include "network.h"

#include <stdio.h>
#include <ctype.h>
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

#include "config.h"
#include "simulation.h"
#include "interface.h"

#ifdef _WIN32
#define close closesocket
#ifndef IPV6_V6ONLY
#define IPV6_V6ONLY 27
#endif
#else
#endif

#define PORT "3490"
#define BACKLOG 4
#define WELCOME "\r\nUse \"n name\" to change name, \"f force\" to change force or \"c\" to clear past shots.\r\nEverything else is interpreted as a shooting angle.\r\n\r\n> "

typedef struct
{
   int socket;
   char msgbuf[128];
   int msgbufindex;
} connection_t;

fd_set master, readfds;
int fdmax, listener;
char buf[256];
char sendbuf[256];
connection_t* connection;

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

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
   if (sa->sa_family == AF_INET)
   {
      return &(((struct sockaddr_in*)sa)->sin_addr);
   }

   return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void initNetwork(void)
{
   struct addrinfo hints, *ai, *p;
   int yes = 1;
   int no = 0;
   int rv;
   
   connection = malloc(conf.maxPlayers * sizeof(connection_t));
   
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

void stepNetwork(void)
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
               for(k = 0; k < conf.maxPlayers; ++k)
               {
                  if(connection[k].socket == 0)
                  {
                     connection[k].socket = newfd;
                     playerJoin(k);
                     updateName(k, "Anonymous");
                     break;
                  }
               }
               if(k == conf.maxPlayers)
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
               for(k = 0; k < conf.maxPlayers; ++k)
               {
                  if(connection[k].socket == i)
                  {
                     connection[k].socket = 0;
                     playerLeave(k);
                     break;
                  }
               }   
               close(i);
               FD_CLR(i, &master);
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
               for(k = 0; k < nbytes && pi >= 0; ++k)
               {
                  unsigned char c = buf[k];
                  //printf("%x ", c);
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
                     if(send(i, connection[pi].msgbuf, strlen(connection[pi].msgbuf), MSG_NOSIGNAL) == -1)
                     {
                        print_error("send");
                     }
                     if(send(i, "\r\n", 2, MSG_NOSIGNAL) == -1)
                     {
                        print_error("send");
                     }
                     printf("%d: \"%s\"\r\n", pi, connection[pi].msgbuf);
                     switch(connection[pi].msgbuf[0])
                     {
                        case 'n':
                        {
                           updateName(pi, connection[pi].msgbuf + 2);
                           break;
                        }
                        case 'f':
                        {
                           updateForce(pi, atof(connection[pi].msgbuf + 2));
                           break;
                        }
                        case 'z':
                        {
                           updateZoom(atof(connection[pi].msgbuf + 2));
                           break;
                        }
                        case 'c':
                        {
                           clearTraces(pi);
                           break;
                        }
                        case 'r':
                        {
                           toggleFps();
                           break;
                        }
                        case 'i':
                        {
                           if(strcmp("init", connection[pi].msgbuf) == 0)
                           {
                              reinitialize();
                           }
                           break;
                        }
                        case 'e':
                        {
                           if(strcmp("exit", connection[pi].msgbuf) == 0)
                           {
                              exit(0);
                           }
                           break;
                        }
                        default:
                        {
                           updateAngle(pi, atof(connection[pi].msgbuf));
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
