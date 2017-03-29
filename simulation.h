#ifndef _SIMULATION_H_
#define _SIMULATION_H_

#include "vector.h"

typedef struct
{
   Vec2d position;
   Vec2d speed;
   int live;
   int leftSource;
   int stale;
} SimMissile;

typedef struct
{
   Vec2f* dot;
   SimMissile missile;
   int length;
   int player;
   double angle;
   double velocity;
} SimShot;

typedef struct
{
   SimShot* shot;
   int currentShot;
   Vec2d position;
   double angle;
   double velocity;
   double energy;
   double oldVelocity;
   int watch;
   int deaths;
   int kills;
   int shots;
   int active;
   int valid;
   int didShoot;
   int timeout;
   int timeoutcnt;
   char name[16];
} SimPlayer;

typedef struct
{
   Vec2d position;
   double radius;
   double mass;
} SimPlanet;

void playerJoin(int p);
void playerLeave(int p);
void updateAngle(int p, double a, int checkEnergy);
void validateOld(int p);
void updateVelocity(int p, double v);
void updateName(int p, char* n);
void tankEnergy(int p);
void clearTraces(int p);
void reinitialize(void);
void toggleWatch(int p);

SimShot* getShot(int p, int s);
SimPlanet* getPlanet(int i);
SimPlayer* getPlayer(int p);
double getFlash(void);
int getCurrentPlayer(void);

void initSimulation(void);
void stepSimulation(void);

double getGPotential(Vec2d pos);
double getPmin();
double getPmax();

#endif /* _SIMULATION_H_ */
