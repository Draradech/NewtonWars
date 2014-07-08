#include "stdlib.h"
#include "time.h"

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

   for(;;)
   {
      stepNetwork();
      stepSimulation();
      stepDisplay();
   };

   return 0;
}
