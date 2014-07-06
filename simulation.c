#include "simulation.h"

#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "config.h"

SimPlanet planet[PLANETS];
SimPlayer player[MAXPLAYERS];
SimBullet bullet;

int currentPlayer;

void (*traceCallback)(void);

static void initPlanets(void)
{
   int i, j;

   for(i = 0; i < PLANETS; i++)
   {
      int nok;
      do
      {
         planet[i].radius = 20.0 + (double)rand() / RAND_MAX * 20.0;
         planet[i].mass = planet[i].radius * planet[i].radius * planet[i].radius / 10.0;
         planet[i].position.x = planet[i].radius + (double)rand() / RAND_MAX * (W - planet[i].radius * 2);
         planet[i].position.y = planet[i].radius + (double)rand() / RAND_MAX * (H - planet[i].radius * 2);
         nok = 0;
         for(j = 0; j < i; ++j)
         {
            if(distance(planet[i].position, planet[j].position) <= (planet[i].radius + planet[j].radius))
            {
               nok = 1;
            }
         }
      } while (nok);
   }
}

static void initBullet(void)
{
  bullet.position = player[currentPlayer].position;
  bullet.speed.x = player[currentPlayer].power * cos(player[currentPlayer].angle / 180.0 * M_PI);
  bullet.speed.y = player[currentPlayer].power * -sin(player[currentPlayer].angle / 180.0 * M_PI);
  bullet.live = 1;
  bullet.leftSource = 0;
  if(traceCallback) traceCallback();
}

void initPlayer(int p, int clear)
{
  int i, nok;

  player[p].angle = 0.0;
  player[p].power = 10.0;
  if(clear)
  {
    player[p].deaths = 0;
    player[p].kills = 0;
    player[p].shots = 0;
  }

  do
  {
    player[p].position.x = 20.0 + (double)rand() / RAND_MAX * (W - 40.0);
    player[p].position.y = 20.0 + (double)rand() / RAND_MAX * (H - 40.0);

    nok = 0;
    for(i = 0; i < PLANETS; ++i)
    {
      if(distance(player[p].position, planet[i].position) <= (100.0 + planet[i].radius))
      {
         nok = 1;
      }
    }
    for(i = 0; i < MAXPLAYERS; ++i)
    {
      if(i == p || !player[p].active) continue;
      if(distance(player[p].position, player[i].position) <= (200.0 + 4.0))
      {
         nok = 1;
      }
    }
  } while (nok);
}

void nextPlayer(void)
{
   int initial = currentPlayer;
   do
   {
      currentPlayer = (currentPlayer + 1) % MAXPLAYERS;
   } while (currentPlayer != initial && !player[currentPlayer].active);
}

static void planetHit(void)
{
   if(traceCallback) traceCallback();
}

static void playerHit(int p)
{
   player[currentPlayer].kills++;
   player[p].deaths++;
   initPlayer(p, 0);
   if(traceCallback) traceCallback();
}

static void wallHit(void)
{
   if(traceCallback) traceCallback();
}

void fireBullet(void)
{
   int s, i, j, p;
   double l;
   Vec2d v;

   initBullet();
   for(s = 0; s < MAXSEGMENTS; ++s)
   {
      for(i = 0; i < INTRES; ++i)
      {
         for(j = 0; j < PLANETS; ++j)
         {
            v = vsub(planet[j].position, bullet.position);
            l = length(v);

            if (l <= planet[j].radius)
            {
               planetHit();
               return;
            }

            v = vdiv(v, l);
            v = vmul(v, planet[j].mass / (l * l));
            v = vdiv(v, INTRES);

            bullet.speed = vadd(bullet.speed, v);
         }
         v = vdiv(bullet.speed, INTRES);
         bullet.position = vadd(bullet.position, v);

         for(p = 0; p < MAXPLAYERS; ++p)
         {
            if(!player[p].active) continue;
            l = distance(player[p].position, bullet.position);

            if (  (l <= 4.0)
               && (bullet.leftSource == 1)
               )
            {
               playerHit(p);
               return;
            }

            if (  (l > 5.0)
               && (p == currentPlayer)
               )
            {
               bullet.leftSource = 1;
            }
         }

         if (  (bullet.position.x < -500)
            || (bullet.position.x > W + 500)
            || (bullet.position.y < -500)
            || (bullet.position.y > H + 500)
            )
         {
            wallHit();
            return;
         }
      }
      if(traceCallback) traceCallback();
   }
}

void setTraceCallback(void (*func)(void))
{
   traceCallback = func;
}

void initSimulation(void)
{
  initPlanets();
}

void stepSimulation(void)
{
}

