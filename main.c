#include <stdlib.h>
#include <time.h>
#ifdef WIN32
#include <windows.h>
#endif

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
   Sleep(millis);
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
      
      if(conf.throttle > 0)
      {
         millisleep(conf.throttle);
      }
   };

   return 0;
}
