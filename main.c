#include "config.h"
#include "network.h"
#include "simulation.h"
#include "interface.h"

int main(int argc, char** argv)
{
   config(&argc, argv);
   
   initNetwork();
   initSimulation();
   initInterface(&argc, argv);
   
   for(;;)
   {
      stepNetwork();
      stepSimulation();
      stepInterface();
   };

   return 0;
}


