#ifndef _CONFIG_H_
#define _CONFIG_H_

#define CONFIG_EXTRAPOINTS_OFF     0
#define CONFIG_EXTRAPOINTS_RANDOM  1
#define CONFIG_EXTRAPOINTS_OLDEST  2
#define CONFIG_EXTRAPOINTS_BEST    3

typedef struct
{
   int maxPlayers;
   int numPlanets;
   int numShots;
   double rate;
   double limit;
   int fullscreen;
   double battlefieldW;
   double battlefieldH;
   double playerSize;
   int margin;
   char* ip;
   char* message;
   int extrapoints;
   int numDebrisParticles;
   double debrisSpeed;
   char sendPlanets;
   /* debug options */
   int fastmode;
   int throttle;
   int debug;
   int pot;
   int area;
   /* fixed */
   int maxSegments;
   int segmentSteps;
   int roundTime;
} Config;

extern Config conf;

void config(int* argc, char** argv);

#endif /* _CONFIG_H_ */
