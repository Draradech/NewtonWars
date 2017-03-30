#include <stdlib.h>
#include <time.h>

#include "config.h"
#include "network.h"
#include "simulation.h"
#include "display.h"

void millisleep(int millis)
{
   #ifndef _WIN32
   struct timespec ts;
   ts.tv_sec = millis / 1000;
   ts.tv_nsec = (millis % 1000) * 1000000;
   nanosleep(&ts, NULL);
   #else
   // todo
   #endif
}

int main(int argc, char** argv)
{
   int i = 0;

   srand(time(0));
   config(&argc, argv);

   initNetwork();
   initSimulation();
   initDisplay(&argc, argv);

   for(;;)
   {
      stepNetwork();
      stepSimulation();
      if(conf.fastmode)
      {
         i++;
         i %= 100;
         if(i != 0) continue;
      }
      stepDisplay();
      millisleep(conf.throttle);
   };

   return 0;
}
