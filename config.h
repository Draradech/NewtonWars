#ifndef _CONFIG_H_
#define _CONFIG_H_

#define LIMIT(x, min, max) (((x) < (min)) ? (min) : ((x) > (max)) ? (max) : (x))

typedef struct
{
   int maxPlayers;
   int numPlanets;
   int maxSegments;
   int segmentSteps;
   int numShots;
   int fastmode;
   int fullscreen;
   int oneline;
   double battlefieldW;
   double battlefieldH;
} Config;

extern Config conf;

void config(int* argc, char** argv);

#endif /* _CONFIG_H_ */

