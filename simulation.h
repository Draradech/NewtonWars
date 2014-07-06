#ifndef _SIMULATION_H_
#define _SIMULATION_H_

#include "vector.h"

typedef struct
{
   Vec2d position;
   double angle;
   double power;
   int deaths;
   int kills;
   int shots;
   int active;
} SimPlayer;

typedef struct
{
   Vec2d position;
   double radius;
   double mass;
} SimPlanet;

typedef struct
{
   Vec2d position;
   Vec2d speed;
   int live;
   int leftSource;
} SimBullet;

extern SimPlanet planet[PLANETS];
extern SimPlayer player[MAXPLAYERS];
extern SimBullet bullet;

extern int currentPlayer;
void fireBullet(void);
void setTraceCallback(void (*func)(void));
void initPlayer(int p, int clear);
void nextPlayer(void);

void initSimulation(void);
void stepSimulation(void);

#endif /* _SIMULATION_H_ */
