#ifndef _SIMULATION_H_
#define _SIMULATION_H_

#include "vector.h"

#define PLANETS 32
#define MAXPLAYERS 6
#define MAXSEGMENTS 2000
#define INTRES 50

/* 16:9 */
/*
#define W 1920.0
#define H 1080.0
*/
/* 4:3 */
#define W 1660.0
#define H 1245.0

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
void init(void);
void initPlayer(int p, int clear);
void nextPlayer(void);

#endif /* _SIMULATION_H_ */
