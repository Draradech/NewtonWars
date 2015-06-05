#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define A 2e6

Config conf;
char ip[64];

void help(void)
{
   printf("\n");
   printf("Usage: nw [OPTION VALUE] [OPTION VALUE] ...\n");
   printf("\n");
   printf("Valid options:\n");
   printf(" players     maximum number of players, default: 12\n");
   printf(" planets     number of planets, default: 32\n");
   /*
   printf(" segments    maximum number of segments per shot, default: 2000\n");
   */
   printf(" steps       simulated substeps per segment, default: 50\n");
   printf(" shots       number of displayed past shots, default: 16\n");
   printf(" fullscreen  enable (1) or disable (0) fullscreen, default: 1\n");
   printf(" timeout     timeout in seconds to enter new valid shot, use 0 to disable, default: 20\n");
   printf(" ratio       aspect ratio of battlefield (1.33, 4:3 and 4/3 are valid formats), default: 4:3\n");
   printf(" ip          if set and a free slot exists, display 'to play, telnet \"ip\" 4390', default: empty\n");
   printf("\n");
}

void config(int* argc, char** argv)
{
   int i;
   char *b, *c;

   conf.maxPlayers = 12;
   conf.numPlanets = 32;
   conf.maxSegments = 2000;
   conf.segmentSteps = 50;
   conf.numShots = 16;
   conf.fullscreen = 1;
   conf.timeout = 20 * 60;
   conf.ip = 0;

   conf.battlefieldW = sqrt(A * 4 / 3); /* 1632 */
   conf.battlefieldH = sqrt(A * 3 / 4); /* 1224 */
   #if (0)
   conf.battlefieldW = sqrt(A * 16 / 9); /* 1885 */
   conf.battlefieldH = sqrt(A * 9 / 16); /* 1060 */
   #endif

   for(i = 1; i < *argc; ++i)
   {
      b = argv[i++];
      c = argv[i];

      if(!c)
      {
         help();
         exit(0);
      }

      if (*b == '-' || *b == '/') b++; /* allow -para and /para */
      if (*b == '-') b++; /* allow --para */

      if (strcmp(b, "players") == 0)
      {
         conf.maxPlayers = atoi(c);
         if(conf.maxPlayers > 12 || conf.maxPlayers < 1)
         {
            printf("players need to be between 1 and 12\n");
            exit(0);
         }
      }
      else if (strcmp(b, "planets") == 0)
      {
         conf.numPlanets = atoi(c);
         if(conf.numPlanets > 64 || conf.numPlanets < 1)
         {
            printf("planets need to be between 1 and 64\n");
            exit(0);
         }
      }
      /*
      else if (strcmp(b, "segments") == 0)
      {
         conf.maxSegments = atoi(c);
         if(conf.maxSegments > 10000 || conf.maxSegments < 100)
         {
            printf("segments need to be between 100 and 10000\n");
            exit(0);
         }
      }
      */
      else if (strcmp(b, "steps") == 0)
      {
         conf.segmentSteps = atoi(c);
         if(conf.segmentSteps > 500 || conf.segmentSteps < 5)
         {
            printf("steps need to be between 5 and 500\n");
            exit(0);
         }
      }
      else if (strcmp(b, "shots") == 0)
      {
         conf.numShots = atoi(c);
         if(conf.numShots > 64 || conf.numShots < 1)
         {
            printf("shots need to be between 1 and 64\n");
            exit(0);
         }
      }
      else if (strcmp(b, "fullscreen") == 0)
      {
         conf.fullscreen = atoi(c);
         if(conf.fullscreen > 1 || conf.fullscreen < 0)
         {
            printf("fullscreen needs to be 0 or 1\n");
            exit(0);
         }
      }
      else if (strcmp(b, "timeout") == 0)
      {
         conf.timeout = atoi(c);
         if(conf.timeout > 120 || conf.timeout < 0)
         {
            printf("timeout needs to be between 0 and 120\n");
            exit(0);
         }
         conf.timeout *= 60;
      }
      else if (strcmp(b, "ratio") == 0)
      {
         double ratio = atof(c);
         char *d;
         d = strstr(c, "/");
         if(d)
         {
            d++;
            ratio /= atof(d);
         }
         else
         {
            d = strstr(c, ":");
            if(d)
            {
               d++;
               ratio /= atof(d);
            }
         }
         if(ratio > 5. || ratio < .5)
         {
            printf("ratio needs to be between 5.0 and 0.5\n");
            exit(0);
         }
         conf.battlefieldW = sqrt(A * ratio);
         conf.battlefieldH = sqrt(A / ratio);
      }
      else if (strcmp(b, "ip") == 0)
      {
         strncpy(ip, c, 63);
         ip[63] = 0;
         if(ip[0])
         {
            conf.ip = ip;
         }
      }
      else if (  (strcmp(b, "h") == 0)
              || (strcmp(b, "help") == 0)
              || (strcmp(b, "?") == 0)
              )
      {
         help();
         exit(0);
      }
      else
      {
         printf("warning: nw ignored parameter %s %s\n", b, c);
      }
   }
}
