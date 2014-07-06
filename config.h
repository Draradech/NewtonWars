#ifndef _CONFIG_H_
#define _CONFIG_H_

#define LIMIT(x, min, max) (((x) < (min)) ? (min) : ((x) > (max)) ? (max) : (x))

typedef struct SConfig
{
   int maxPlayers;
   int numPlanets;
   int maxSegments;
   int segmentSteps;
   int fastmode;
   int fullscreen;
   double battlefieldW;
   double battlefieldH;
} TConfig;

extern TConfig conf;

void config(int* argc, char** argv);

#endif /* _CONFIG_H_ */

