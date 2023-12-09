#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "config.h"

#define A 2e6

Config conf;

void help(void)
{
   printf("\n");
   printf("Usage: nw [OPTION VALUE] [OPTION VALUE] ...\n");
   printf("\n");
   printf("Valid options:\n");
   printf(" players       maximum number of players (default: 12)\n");
   printf(" planets       number of planets (default: 24)\n");
   printf(" shots         number of displayed past shots (default: 16)\n");
   printf(" rate          energy increase rate (default 2.0/s)\n");
   printf(" limit         energy limit (default 200.0)\n");
   printf(" roundtime     time limit per round in s (default 300)\n");
   printf(" extrapoints   selects the extrapoint mode (one out of: off, random, oldest, best\n) (default: best)");
   printf(" numdebris     number of debris particles on kill (default 10)\n");
   printf(" speeddebris   speed of debris particles (default 3.0)\n");
   printf(" sendplanets   send planet positions to bots (0 / 1) (default 0)\n");
   printf("\n");
   printf(" fullscreen    fullscreen enabled (default: 1)\n");
   printf(" ratio         aspect ratio of battlefield (1.33, 4:3 and 4/3 are valid formats) (default: 16:9)\n");
   printf(" playersize    radius of players (default: 4.0)\n");
   printf(" margin        margin around the battlefield before shots are voided (default: 500.0)\n");
   printf("\n");
   printf(" ip            if set and a free slot exists, display 'to play, telnet \"ip\" 3490' (default: empty)\n");
   printf(" message       if set and a free slot exists, display message, supersedes ip (default: empty)\n");
   printf("\n");
   printf("\n");
   printf("Debug options\n");
   printf(" debug         debug output, default: 0\n");
   printf(" fastmode      render only every 100th frame, default: 0\n");
   printf(" throttle      delay for every step in ms, default: 0.0\n");
   printf("\n");
}

void config(int* argc, char** argv)
{
   int i;
   char *b, *c;

   // changeable via cmd line
   conf.maxPlayers = 12;
   conf.numPlanets = 24;
   conf.numShots = 16;
   conf.rate = 2.0;
   conf.limit = 200.01;
   conf.roundTime = 300; 

   conf.fullscreen = 1;
   conf.battlefieldW = sqrt(A * 16 / 9); /* 1885 */
   conf.battlefieldH = sqrt(A * 9 / 16); /* 1060 */
   conf.playerSize = 4.0;
   conf.margin = 500;

   conf.ip = 0;
   conf.message = 0;

   conf.extrapoints = CONFIG_EXTRAPOINTS_BEST;
   
   conf.numDebrisParticles = 10;
   conf.debrisSpeed = 3.0;
   conf.sendPlanets = 0;
   
   //debug
   conf.debug = 0;
   conf.fastmode = 0;
   conf.throttle = 0;
   
   //fixed
   conf.maxSegments = 2000;
   conf.segmentSteps = 25;

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
      else if (strcmp(b, "shots") == 0)
      {
         conf.numShots = atoi(c);
         if(conf.numShots > 64 || conf.numShots < 1)
         {
            printf("shots need to be between 1 and 64\n");
            exit(0);
         }
      }
      else if (strcmp(b, "rate") == 0)
      {
         conf.rate = atof(c);
         if(conf.rate > 10.0 || conf.rate < 0.1)
         {
            printf("rate needs to be >= 0.1 and <= 10.0\n");
            exit(0);
         }
      }
      else if (strcmp(b, "limit") == 0)
      {
         conf.limit = atof(c);
         if(conf.limit > 10000.0 || conf.limit < 10.0)
         {
            printf("limit needs to be >= 0.0 and <= 10.0\n");
            exit(0);
         }
      }
      else if (strcmp(b, "roundtime") == 0)
      {
         conf.roundTime = atof(c);
         if(conf.roundTime > 3600 || conf.limit < 0)
         {
            printf("roundtime needs to be >= 0 and <= 3600\n");
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
      else if (strcmp(b, "playersize") == 0)
      {
         conf.playerSize = atof(c);
         if(conf.playerSize > 10 || conf.playerSize <= 0)
         {
            printf("playersize needs to be > 0.0 and <= 10.0\n");
            exit(0);
         }
      }
      else if (strcmp(b, "margin") == 0)
      {
         conf.margin = atoi(c);
         if(conf.margin < 0 || conf.margin > 10000)
         {
            printf("margin needs to be between 0 and 10000\n");
            exit(0);
         }
      }
      else if (strcmp(b, "ip") == 0)
      {
         if(c[0])
         {
            conf.ip = c;
         }
      }
      else if (strcmp(b, "message") == 0)
      {
         if(c[0])
         {
            conf.message = c;
         }
      }
      else if (strcmp(b, "sendplanets") == 0)
      {
         conf.sendPlanets = atoi(c);
         if (conf.sendPlanets < 0 || conf.sendPlanets > 1)
         {
            printf("sendplanets needs to be 0 or 1\n");
            exit(0);
         }
      }
      else if (strcmp(b, "debug") == 0)
      {
         conf.debug = atoi(c);
         if(conf.debug > 1 || conf.debug < 0)
         {
            printf("debug needs to be 0 or 1\n");
            exit(0);
         }
      }
      else if (strcmp(b, "fastmode") == 0)
      {
         conf.fastmode = atoi(c);
         if(conf.fastmode > 1 || conf.fastmode < 0)
         {
            printf("fastmode needs to be 0 or 1\n");
            exit(0);
         }
      }
      else if (strcmp(b, "throttle") == 0)
      {
         conf.throttle = atoi(c);
         if(conf.throttle > 10000 || conf.throttle < 0)
         {
            printf("throttle needs to be >= 0 and <= 10000\n");
            exit(0);
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
      else if ( (strcmp(b, "extrapoints") == 0) )
      {
          if( strcmp(c,"random") == 0 )
          {
             conf.extrapoints = CONFIG_EXTRAPOINTS_RANDOM;
          }
          else if( strcmp(c,"oldest") == 0 )
          {
             conf.extrapoints = CONFIG_EXTRAPOINTS_OLDEST;
          }
          else if( strcmp(c,"best") == 0 )
          {
             conf.extrapoints = CONFIG_EXTRAPOINTS_BEST;
          }
          else
          {
             conf.extrapoints = CONFIG_EXTRAPOINTS_OFF;
          }
      }
      else
      {
         printf("warning: nw ignored parameter %s %s\n", b, c);
      }
   }
}
