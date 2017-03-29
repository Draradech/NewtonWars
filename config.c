#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define A 2e6

Config conf;

void help(void)
{
   printf("\n");
   printf("Usage: nw [OPTION VALUE] [OPTION VALUE] ...\n");
   printf("\n");
   printf("Valid options:\n");
   printf(" players       maximum number of players, default: 12\n");
   printf(" planets       number of planets, default: 24\n");
   printf(" shots         number of displayed past shots, default: 16\n");
   printf(" fullscreen    fullscreen enabled, default: 1\n");
   printf(" fastmode      render only every 1000th frame, default: 0\n");
   printf(" timeout       timeout in seconds to enter new valid shot, use 0 to disable, default: 30\n");
   printf(" ratio         aspect ratio of battlefield (1.33, 4:3 and 4/3 are valid formats), default: 16:9\n");
   printf(" ip            if set and a free slot exists, display 'to play, telnet \"ip\" 3490', default: empty\n");
   printf(" message       if set and a free slot exists, display message, supesedes ip, default: empty\n");
   printf(" energy        limit available energy (default: 1)\n");
   printf(" realtime      realtime mode (implies energy, default: 1)\n");
   printf(" rate          energy increase rate (default 1.0 / s)\n");
   printf(" playersize    radius of players (default: 4.0)\n");
   printf("\n");
   printf("Margins around the battlefield before shots are voided (default: 500.0)\n");
   printf(" marginleft\n");
   printf(" marginright\n");
   printf(" margintop\n");
   printf(" marginbottom\n");
   printf(" margins       set all margins at once\n");
   printf("\n");
   printf("DEBUG options\n");
   printf(" throttle      delay for every step in ms, default: 0.0\n");
   printf(" debug         debug output, default: 0\n");
   printf("\n");
}

void config(int* argc, char** argv)
{
   int i;
   char *b, *c;

   conf.maxPlayers = 12;
   conf.numPlanets = 24;
   conf.maxSegments = 2000;
   conf.segmentSteps = 50;
   conf.numShots = 16;
   conf.fullscreen = 1;
   conf.fastmode = 0;
   conf.playerSize = 4.0;
   conf.timeout = 30 * 60;
   conf.ip = 0;
   conf.energy = 1;
   conf.realtime = 1;
   conf.rate = 1.0;

   conf.throttle = 0;
   conf.debug = 0;

   conf.margintop = conf.marginleft = conf.marginright = conf.marginbottom = 500;

   conf.battlefieldW = sqrt(A * 16 / 9); /* 1885 */
   conf.battlefieldH = sqrt(A * 9 / 16); /* 1060 */

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
      else if (strcmp(b, "segments") == 0)
      {
         conf.maxSegments = atoi(c);
         if(conf.maxSegments > 10000 || conf.maxSegments < 100)
         {
            printf("segments need to be between 100 and 10000\n");
            exit(0);
         }
      }
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
      else if (strcmp(b, "margins") == 0)
      {
         conf.margintop = conf.marginleft = conf.marginright = conf.marginbottom = atoi(c);
         if(conf.margintop < 0 || conf.margintop > 10000)
         {
            printf("margins need to be between 0 and 10000\n");
            exit(0);
         }
      }
      else if (strcmp(b, "margintop") == 0)
      {
         conf.margintop = atoi(c);
         if(conf.margintop < 0 || conf.margintop > 10000)
         {
            printf("margintop need to be between 0 and 10000\n");
            exit(0);
         }
      }
      else if (strcmp(b, "marginleft") == 0)
      {
         conf.marginleft = atoi(c);
         if(conf.marginleft < 0 || conf.marginleft > 10000)
         {
            printf("marginleft need to be between 0 and 10000\n");
            exit(0);
         }
      }
      else if (strcmp(b, "marginright") == 0)
      {
         conf.marginright = atoi(c);
         if(conf.marginright < 0 || conf.marginright > 10000)
         {
            printf("marginright need to be between 0 and 10000\n");
            exit(0);
         }
      }
      else if (strcmp(b, "marginbottom") == 0)
      {
         conf.marginbottom = atoi(c);
         if(conf.marginbottom < 0 || conf.marginbottom > 10000)
         {
            printf("marginbottom need to be between 0 and 10000\n");
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
      else if (strcmp(b, "energy") == 0)
      {
         conf.energy = atoi(c);
         if(conf.energy > 1 || conf.energy < 0)
         {
            printf("energy needs to be 0 or 1\n");
            exit(0);
         }
      }
      else if (strcmp(b, "realtime") == 0)
      {
         conf.realtime = atoi(c);
         if(conf.realtime > 1 || conf.realtime < 0)
         {
            printf("realtime needs to be 0 or 1\n");
            exit(0);
         }
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
      else if (strcmp(b, "rate") == 0)
      {
         conf.rate = atof(c);
         if(conf.rate > 10.0 || conf.rate < 0.1)
         {
            printf("rate needs to be >= 0.1 and <= 10.0\n");
            exit(0);
         }
      }
      else if (strcmp(b, "throttle") == 0)
      {
         conf.throttle = atoi(c);
         if(conf.throttle > 10000 || conf.throttle < 0)
         {
            printf("playersize needs to be >= 0 and <= 10000\n");
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
   if (conf.realtime)
   {
      conf.energy = 1;
      conf.timeout = 0;
   }
}
