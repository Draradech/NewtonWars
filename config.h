#ifndef _CONFIG_H_
#define _CONFIG_H_

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
   /* debug options */
   int fastmode;
   int throttle;
   int debug;
   int pot;
   int area;
   /* fixed */
   int maxSegments;
   int segmentSteps;
} Config;

extern Config conf;

void config(int* argc, char** argv);

#endif /* _CONFIG_H_ */
