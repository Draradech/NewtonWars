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
   int deaths;
   int kills;
   int active;
   int valid;
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
void updateAngle(int p, double a, int ce);
void updateVelocity(int p, double v);
void updateName(int p, char* n);
void tankEnergy(int p);
void clearTraces(int p);
void reinitialize(void);

SimShot* getShot(int p, int s);
SimPlanet* getPlanet(int i);
SimPlayer* getPlayer(int p);
double getFlash(void);

void initSimulation(void);
void stepSimulation(void);

double getGPot(int x, int y);
double getPmin();
double getPmax();

#endif /* _SIMULATION_H_ */
