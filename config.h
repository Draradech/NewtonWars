#ifndef _CONFIG_H_
#define _CONFIG_H_

#define LIMIT(x, min, max) (((x) < (min)) ? (min) : ((x) > (max)) ? (max) : (x))

#include <time.h>

// define some game configuration options

#define GM_ENERGY 0x0001
#define GM_PARALLEL 0x0002
#define GM_MULTIMISSILE 0x0004
#define GM_OVERDRIVE 0x0008

typedef struct
{
   int maxPlayers;
   int numPlanets;
   int maxSegments;
   int segmentSteps;
   int numShots;
   int fastmode;
   int fullscreen;
   int timeout;
   int margintop;
   int marginleft;
   int marginright;
   int marginbottom;
   double playerDiameter;
   int mode;
   int debug;
   double battlefieldW;
   double battlefieldH;
   struct timespec throttle;
   char* ip;
} Config;

extern Config conf;

void config(int* argc, char** argv);

#endif /* _CONFIG_H_ */
