#include <stdlib.h>
#include <time.h>

#include "config.h"
#include "network.h"
#include "simulation.h"
#include "display.h"

int main(int argc, char** argv)
{
   srand(time(0));
   config(&argc, argv);

   initNetwork();
   initSimulation();
   initDisplay(&argc, argv);

   int i = 0;
   for(;;)
   {
      stepNetwork();
      stepSimulation();
      if(overdrive)
      {
         i++;
         i %= 1000;
         if(i != 0) continue;
      }
      stepDisplay();
      nanosleep(&conf.throttle,NULL);
   };

   return 0;
}
