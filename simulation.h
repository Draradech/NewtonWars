#ifndef _SIMULATION_H_
#define _SIMULATION_H_

#include "vector.h"

typedef struct
{
   Vec2d position;
   Vec2d speed;
   int live;
   int leftSource;
} SimMissile;

typedef struct
{
   Vec2d* dot;
   SimMissile missile;
   int length;
} SimShot;

typedef struct
{
   SimShot* shot;
   int currentShot;
   Vec2d position;
   double angle;
   double force;
   int deaths;
   int kills;
   int shots;
   int active;
   int valid;
   int didShoot;
} SimPlayer;

typedef struct
{
   Vec2d position;
   double radius;
   double mass;
} SimPlanet;

void playerJoin(int p);
void playerLeave(int p);
void updateAngle(int p, double a);
void updateForce(int p, double f);
void clearTraces(int p);
void reinitialize(void);

SimShot* getShot(int p, int s);
SimPlanet* getPlanet(int i);
SimPlayer* getPlayer(int p);
int getCurrentPlayer(void);

void initSimulation(void);
void stepSimulation(void);

#endif /* _SIMULATION_H_ */
