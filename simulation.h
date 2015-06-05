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
   Vec2f* dot;
   SimMissile missile;
   int length;
   int player;
} SimShot;

typedef struct
{
   SimShot* shot;
   int currentShot;
   Vec2d position;
   double angle;
   double velocity;
   double oldVelocity;
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
void updateAngle(int p, double a);
void validateOld(int p);
void updateVelocity(int p, double v);
void updateName(int p, char* n);
void clearTraces(int p);
void reinitialize(void);

SimShot* getShot(int p, int s);
SimPlanet* getPlanet(int i);
SimPlayer* getPlayer(int p);
double getFlash(void);
int getCurrentPlayer(void);
int getDeathMessage(char* buf);

void initSimulation(void);
void stepSimulation(void);

#endif /* _SIMULATION_H_ */
