#include "simulation.h"

#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "config.h"

static SimPlanet* planet;
static SimPlayer* player;
static int currentPlayer;

static void initPlanets(void)
{
   int i, j;

   for(i = 0; i < conf.numPlanets; i++)
   {
      int nok;
      do
      {
         planet[i].radius = 20.0 + (double)rand() / RAND_MAX * 20.0;
         planet[i].mass = planet[i].radius * planet[i].radius * planet[i].radius / 10.0;
         planet[i].position.x = planet[i].radius + (double)rand() / RAND_MAX * (conf.battlefieldW - planet[i].radius * 2);
         planet[i].position.y = planet[i].radius + (double)rand() / RAND_MAX * (conf.battlefieldH - planet[i].radius * 2);
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

static void initPlayer(int p, int clear)
{
   int i, sh, nok;

   if(clear)
   {
      player[p].angle = 0.0;
      player[p].force = 10.0;
      player[p].deaths = 0;
      player[p].kills = 0;
      player[p].shots = 0;
   }

   player[p].active = 1;
   player[p].currentShot = 0;
   player[p].valid = 0;
   player[p].didShoot = 0;

   for(sh = 0; sh < conf.numShots; ++sh)
   {
      SimShot* s = &(player[p].shot[sh]);
      s->length = 0;
      s->missile.live = 0;
   }

   do
   {
      player[p].position.x = 20.0 + (double)rand() / RAND_MAX * (conf.battlefieldW - 40.0);
      player[p].position.y = 20.0 + (double)rand() / RAND_MAX * (conf.battlefieldH - 40.0);

      nok = 0;
      for(i = 0; i < conf.numPlanets; ++i)
      {
         if(distance(player[p].position, planet[i].position) <= (100.0 + planet[i].radius))
         {
            nok = 1;
         }
      }
      for(i = 0; i < conf.maxPlayers; ++i)
      {
         if(i == p || !player[p].active) continue;
         if(distance(player[p].position, player[i].position) <= (200.0 + 4.0))
         {
            nok = 1;
         }
      }
   } while (nok);
}

static void nextPlayer(void)
{
   int initial = currentPlayer;
   do
   {
      currentPlayer = (currentPlayer + 1) % conf.maxPlayers;
   } while (currentPlayer != initial && !player[currentPlayer].active);
   player[currentPlayer].didShoot = 0;
}

static void missileEnd(SimShot* s)
{
   s->missile.live = 0;
   s->dot[s->length++] = s->missile.position;
}

static void planetHit(SimShot* s)
{
   missileEnd(s);
}

static void playerHit(SimShot* s, int p)
{
   int pl;
   player[currentPlayer].kills++;
   player[p].deaths++;
   initPlayer(p, 0);
   missileEnd(s);
   for(pl = 0; pl < conf.maxPlayers; ++pl)
   {
      player[pl].valid = 0;
   }
}

static void wallHit(SimShot* s)
{
   missileEnd(s);
}

static void initShot(int pl)
{
   SimPlayer*  p = &(player[pl]);
   SimShot*    s = &(p->shot[p->currentShot]);
   SimMissile* m = &(s->missile);
   
   m->position = p->position;
   m->speed.x = p->force * cos(p->angle / 180.0 * M_PI);
   m->speed.y = p->force * -sin(p->angle / 180.0 * M_PI);
   m->live = 1;
   m->leftSource = 0;
   s->dot[0] = m->position;
   s->length = 1;
}

static void simulate(void)
{
   int sh, i, j, pl, pl2;
   double l;
   Vec2d v;

   for(i = 0; i < conf.segmentSteps; ++i)
   {
      for(pl = 0; pl < conf.maxPlayers; ++pl)
      {
         if(!player[pl].active) continue;
         SimPlayer* p = &(player[pl]);
         for(sh = 0; sh < conf.numShots; ++sh)
         {
            if(!p->shot[sh].missile.live) continue;
            SimShot* s = &(p->shot[sh]);
            SimMissile* m = &(s->missile);
            for(j = 0; j < conf.numPlanets; ++j)
            {
               v = vsub(planet[j].position, m->position);
               l = length(v);

               if (l <= planet[j].radius)
               {
                  planetHit(s);
               }

               v = vdiv(v, l);
               v = vmul(v, planet[j].mass / (l * l));
               v = vdiv(v, conf.segmentSteps);

               m->speed = vadd(m->speed, v);
            }
            v = vdiv(m->speed, conf.segmentSteps);
            m->position = vadd(m->position, v);

            for(pl2 = 0; pl2 < conf.maxPlayers; ++pl2)
            {
               if(!player[pl2].active) continue;
               l = distance(player[pl2].position, m->position);

               if (  (l <= 4.0)
                  && (m->leftSource == 1)
                  )
               {
                  playerHit(s, pl2);
               }

               if (  (l > 5.0)
                  && (pl2 == pl)
                  )
               {
                  m->leftSource = 1;
               }
            }

            if (  (m->position.x < -500)
               || (m->position.x > conf.battlefieldW + 500)
               || (m->position.y < -500)
               || (m->position.y > conf.battlefieldH + 500)
               )
            {
               wallHit(s);
            }
         }
      }
   }
   for(pl = 0; pl < conf.maxPlayers; ++pl)
   {
      if(!player[pl].active) continue;
      SimPlayer* p = &(player[pl]);
      for(sh = 0; sh < conf.numShots; ++sh)
      {
         if(!p->shot[sh].missile.live) continue;
         SimShot* s = &(p->shot[sh]);
         s->dot[s->length++] = s->missile.position;
         if(s->length == conf.maxSegments)
         {
            s->missile.live = 0;
         }
      }
   }
}

void initSimulation(void)
{
   int sh, pl;
   
   planet = malloc(conf.numPlanets * sizeof(SimPlanet));
   initPlanets();
   player = malloc(conf.maxPlayers * sizeof(SimPlayer));
   for(pl = 0; pl < conf.maxPlayers; ++pl)
   {
      SimPlayer* p = &(player[pl]);
      p->shot = malloc(conf.numShots * sizeof(SimShot));
      for(sh = 0; sh < conf.numShots; ++sh)
      {
         SimShot* s = &(p->shot[sh]);
         s->dot = malloc(conf.maxSegments * sizeof(Vec2d));
         s->missile.live = 0;
         s->length = 0;
      }
      p->active = 0;
   }
}

void stepSimulation(void)
{
   if(player[currentPlayer].active && player[currentPlayer].shot[player[currentPlayer].currentShot].missile.live == 0 && player[currentPlayer].didShoot)
   {
      nextPlayer();
   }
   if(player[currentPlayer].valid && !player[currentPlayer].didShoot)
   {
      player[currentPlayer].valid = 0;
      player[currentPlayer].didShoot = 1;
      player[currentPlayer].currentShot = (player[currentPlayer].currentShot + 1) % conf.numShots;
      initShot(currentPlayer);
   }
   simulate();
}

void playerJoin(int p)
{
   initPlayer(p, 1);
}

void playerLeave(int p)
{
   player[p].active = 0;
}

void updateAngle(int p, double a)
{
   player[p].angle = a;
   player[p].valid = 1;  
}

void updateForce(int p, double f)
{
   player[p].force = f;
}

void clearTraces(int p)
{
   int sh;
   for(sh = 0; sh < conf.numShots; ++sh)
   {
      SimShot* s = &(player[p].shot[sh]);
      if(!s->missile.live)
      {
         s->length = 0;
      }
   }
}

void reinitialize(void)
{
   int pl;
   initPlanets();
   for(pl = 0; pl < conf.maxPlayers; ++pl)
   {
      if(player[pl].active)
      {
         initPlayer(pl, 1);
      }
   }
}

SimShot* getShot(int p, int s)
{
   return &(player[p].shot[((player[p].currentShot + conf.numShots - s) % conf.numShots)]);
}

SimPlanet* getPlanet(int i)
{
   return &(planet[i]);
}

SimPlayer* getPlayer(int p)
{
   return &(player[p]);
}

int getCurrentPlayer(void)
{
   return currentPlayer;
}

