#ifndef _CONFIG_H_
#define _CONFIG_H_

#define CONFIG_GAMETYPE_DEFAULT         0
#define CONFIG_GAMETYPE_PREFERED_TARGET 1
#define CONFIG_GAMETYPE_KILL_OLDEST     2
#define CONFIG_GAMETYPE_KILL_BEST       3

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
   int gametype;
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
