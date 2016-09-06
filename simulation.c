#include "simulation.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "config.h"
#include "network.h"

static SimPlanet* planet;
static SimPlayer* player;
static int currentPlayer;
static double killflash;
static double pmin, pmax;

static void initPlanets(void)
{
   int tries = 0;
   do
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

      tries++;
   }
   while (!potentialEvaluation());

   printf("pmin: %.2lf pmax: %.2lf (%d tries)\n", pmin, pmax, tries);
}

double potential[160][120];
char area[160][120];

static void floodfill(i, j)
{
   if(!area[i][j] && potential[i][j] > pmin)
   {
      area[i][j] = 1;
      if(i > 0) floodfill(i - 1, j);
      if(j > 0) floodfill(i, j - 1);
      if(i < 159) floodfill(i + 1, j);
      if(j < 119) floodfill(i, j + 1);
   }
}

static int potentialEvaluation(void)
{
   int i, j, found;
   Vec2d p;
   double sum = 0;
   int num = 0;

   for(i = 0; i < 160; ++i)
   {
      p.x = (i - 20.0) * conf.battlefieldW / 120;
      for(j = 0; j < 120; ++j)
      {
         p.y = (j - 20.0) * conf.battlefieldH / 80;
         potential[i][j] = getGPotential(p);
         area[i][j] = 0;
         if (potential[i][j] > 0.1)
         {
            sum += potential[i][j];
            num++;
         }
      }
   }

   pmin = 0.9 * sum / num;
   pmax = pmin + 40;

   found = 0;
   for(i = 0; i < 160 && !found; ++i)
   {
      for(j = 0; j < 120 && !found; ++j)
      {
         if(potential[i][j] > pmin) found = 1;
      }
   }

   floodfill(i, j);

   for(i = 0; i < 160; ++i)
   {
      for(j = 0; j < 120; ++j)
      {
         if(!area[i][j] && potential[i][j] > pmin) return 0;
      }
   }

   return 1;
}

static void initPlayer(int p, int clear)
{
   int i, j, k, nok, tries;

   if(clear)
   {
      player[p].angle = 0.0;
      player[p].energy = 25.0;
      player[p].velocity = 10.0;
      player[p].oldVelocity = 10.0;
      player[p].deaths = 0;
      player[p].kills = 0;
      player[p].shots = 0;
      player[p].watch = 0;
      player[p].currentShot = 0;
   }

   player[p].active = 1;
   player[p].valid = 0;
   player[p].didShoot = 0;
   player[p].timeout = conf.timeout;
   player[p].timeoutcnt = 0;

   for(i = 0; i < conf.numShots; ++i)
   {
      SimShot* s = &(player[p].shot[i]);
      if(!s->missile.live)
      {
         s->length = 0;
      }
      else
      {
         s->missile.stale = 1;
      }
   }

   tries = 0;
   do
   {
      player[p].position.x = 20.0 + (double)rand() / RAND_MAX * (conf.battlefieldW - 40.0);
      player[p].position.y = 20.0 + (double)rand() / RAND_MAX * (conf.battlefieldH - 40.0);

      nok = 0;
      if(getGPotential(player[p].position) > pmax || getGPotential(player[p].position) < pmin)
      {
         nok = 1;
      }
      for(i = 0; i < conf.maxPlayers; ++i)
      {
         if(i == p || !player[i].active) continue;
         if(distance(player[p].position, player[i].position) <= (200.0 + 4.0 + 4.0))
         {
            nok = 1;
         }
      }
      for(i = 0; i < conf.numPlanets; ++i)
      {
         if(distance(player[p].position, planet[i].position) <= (planet[i].radius + 4.0))
         {
            nok = 1;
         }
      }
      for(i = 0; i < conf.maxPlayers && tries < 2000 && !nok; ++i)
      {
         if(i == p || !player[i].active) continue;
         for(j = 0; j < conf.numShots && !nok; ++j)
         {
            for(k = 0; k < player[i].shot[j].length && !nok; ++k)
            {
               if(distance(player[p].position, f2d(player[i].shot[j].dot[k])) <= 100.0)
               {
                  nok = 1;
               }
            }
         }
      }
      tries++;
   } while (nok);
   if(tries >= 2000)
   {
      printf("Couldn't keep player spawn away from existing shots.\n");
   }
}

static void nextPlayer(void)
{
   int initial = currentPlayer;
   if(conf.energy)
   {
     player[currentPlayer].energy += 10.0;
   }
   do
   {
      currentPlayer = (currentPlayer + 1) % conf.maxPlayers;
   } while (currentPlayer != initial && !player[currentPlayer].active && !player[currentPlayer].watch);
   player[currentPlayer].didShoot = 0;
   if(player[currentPlayer].watch)
   {
     player[currentPlayer].didShoot = 1;
   }
}

static void missileEnd(SimShot* s)
{
   s->missile.live = 0;
   s->dot[s->length++] = d2f(s->missile.position);
   allSendShotFinished(s);
   if(s->missile.stale)
   {
      s->length = 0;
   }
}

static void planetHit(SimShot* s)
{
   missileEnd(s);
}

static void playerHit(SimShot* s, int p, int p2)
{
   int pl;
   if(p == p2)
   {
     player[p].deaths++;
   }
   else
   {
     player[p].kills++;
     player[p2].deaths++;
   }
   missileEnd(s);
   initPlayer(p2, 0);
   allSendPlayerPos(p2);
   allSendKillMessage(p, p2);
   for(pl = 0; pl < conf.maxPlayers; ++pl)
   {
      if(!conf.energy)
      {
        player[pl].velocity = 10.0;
      }
      killflash = 1.0;
   }
   nextPlayer(); /* not nice here, think about this more (why is this neccessary again?)*/
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
   if(conf.energy)
   {
      p->energy -= p->velocity;
   }
   m->speed.x = p->velocity * cos(p->angle / 180.0 * M_PI);
   m->speed.y = p->velocity * -sin(p->angle / 180.0 * M_PI);
   m->live = 1;
   m->leftSource = 0;
   m->stale = 0;
   s->dot[0] = d2f(m->position);
   s->length = 1;
   s->player = pl;
   s->angle = p->angle;
   s->velocity = p->velocity;
   allSendShotBegin(s);
}

static void simulate(void)
{
   int sh, i, j, pl, pl2, actp;
   double l;
   Vec2d v;
   
   for(i = 0; i < conf.segmentSteps; ++i)
   {
      for(pl = 0; pl < conf.maxPlayers; ++pl)
      {
         SimPlayer* p = &(player[pl]);
         if(p->watch) continue;
         if(!p->active) continue;
         for(sh = 0; sh < conf.numShots; ++sh)
         {
            SimShot* s = &(p->shot[sh]);
            SimMissile* m = &(s->missile);
            if(!m->live) continue;
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

               if (  (l <= conf.playerSize)
                  && (m->leftSource == 1)
                  )
               {
                  if(conf.debug) printf("l = %.5f playerSize = %.5f missile.x = %.5f missile.y = %.5f player.x = %5f player.y = %5f\n",l,conf.playerSize,m->position.x,m->position.y,player[pl2].position.x,player[pl2].position.y);
                  playerHit(s, pl, pl2);
               }

               if (  (l > (conf.playerSize + 1))
                  && (pl2 == pl)
                  )
               {
                  m->leftSource = 1;
               }
            }

            if (  (m->position.x < -conf.marginleft)
               || (m->position.x > conf.battlefieldW + conf.marginright)
               || (m->position.y < -conf.margintop)
               || (m->position.y > conf.battlefieldH + conf.marginbottom)
               )
            {
               wallHit(s);
            }
         }
      }
   }
   for(pl = 0, actp = 0; pl < conf.maxPlayers; ++pl) actp += player[pl].active;  
   for(pl = 0; pl < conf.maxPlayers; ++pl)
   {
      SimPlayer* p = &(player[pl]);
      if(!p->active) continue;
      if(p->watch) continue;
      if(p->timeout) p->timeout--;
      if(p->valid || actp == 1) p->timeout = conf.timeout;
      for(sh = 0; sh < conf.numShots; ++sh)
      {
         SimShot* s = &(p->shot[sh]);
         if(!s->missile.live) continue;
         p->timeout = conf.timeout;
         p->timeoutcnt = 0;
         s->dot[s->length++] = d2f(s->missile.position);
         if(s->length == conf.maxSegments)
         {
            s->missile.live = 0;
            allSendShotFinished(s);
         }
      }
   }
}

double getGPotential(Vec2d pos)
{
   double l, potential = 0;
   int j;

   for(j = 0; j < conf.numPlanets; ++j)
   {
      Vec2d v;
      v = vsub(planet[j].position, pos);
      l = length(v);

      if (l <= planet[j].radius)
      {
         return 0;
      }

      potential += planet[j].mass / l;
   }
   return potential;
}

double getPmin() { return pmin; }
double getPmax() { return pmax; }

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
         s->dot = malloc(conf.maxSegments * sizeof(Vec2f));
         s->missile.live = 0;
         s->length = 0;
      }
      p->active = 0;
   }
   killflash = 0.0;
}

void stepSimulation(void)
{
   int pl;
   for(pl = 0; pl < conf.maxPlayers; ++pl)
   {
      SimPlayer* p = &(player[pl]);

      if (  (conf.debug)
         && (  (pl == currentPlayer)
            || (conf.realtime)
            )
         )
      {
         printf("stepSimulation: %s:%d player=%d player.energy=%.4f player.velocity=%.4f\n", __FILE__, __LINE__, pl, p->energy, p->velocity);
      }

      if (  (!conf.realtime)
         && (pl == currentPlayer)
         && (  (  (  (p->active)
                  && (p->shot[p->currentShot].missile.live == 0)
                  && (p->didShoot)
                  )
               || (  (p->energy < p->velocity)
                  && (conf.energy)
                  && (!p->didShoot)
                  && (p->valid)
                  )
               )
            || (  (p->timeout == 0)
               && (conf.timeout)
               )
            || (p->watch)
            )
         )
      {
         if (  (p->timeout == 0)
            && (!p->watch)
            )
         {
            p->timeoutcnt++;
         }
         if(conf.debug) printf("stepSimulation: %s:%d call nextPlayer\n",__FILE__,__LINE__);
         nextPlayer();
      }

      if (  (  (conf.realtime)
            || (pl == currentPlayer)
            )
         && (p->active)
         && (p->valid)
         && (!p->didShoot)
         && (  (p->energy >= p->velocity)
            || (!conf.energy)
            )
         )
      {
         p->currentShot = (p->currentShot + 1) % conf.numShots;
         initShot(pl);
         p->valid = 0;
         if(!conf.energy)
         {
            p->velocity = 10.0;
            p->oldVelocity = 10.0;
         }
         if(!conf.realtime)
         {
            p->didShoot = 1;
         }
      }
      if(conf.realtime)
      {
         p->energy += 10.0 / 10.0 / 60.0;
      }
   }
   simulate();
   killflash *= 0.95;
}

void playerJoin(int p)
{
   int pi;
   initPlayer(p, 1);
   if(p == 0)
   {
      for(pi = 1; pi < conf.maxPlayers; pi++)
      {
         if(player[pi].active)
         {
            break;
         }
      }
      if(pi == conf.maxPlayers)
      {
         currentPlayer = 0;
      }
   }
}

void playerLeave(int p)
{
   player[p].active = 0;
   if(!conf.realtime && p == currentPlayer)
   {
      nextPlayer();
   }
   killflash = 1.0;
}

void updateAngle(int p, double a)
{
   if(!((a > -720.0) && (a < 720.0))) a = 0.0;
   player[p].angle = a;
   player[p].valid = 1;  
}

void validateOld(int p)
{
   player[p].valid = 1;  
   player[p].velocity = player[p].oldVelocity;
}

void toggleWatch(int p)
{
  player[p].watch ^= 1;
}

void updateVelocity(int p, double v)
{
   double limit = conf.energy ? 50.0 : 15.0;
   player[p].velocity = LIMIT(v, 0.0, limit);
   player[p].oldVelocity = LIMIT(v, 0.0, limit);
}

void tankEnergy(int p)
{
   if(p >= 0 && p < conf.maxPlayers) player[p].energy += 100;
}

void updateName(int p, char* n)
{
   strncpy(player[p].name, n, 15);
   player[p].name[15] = 0;
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
         allSendPlayerPos(pl);
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

double getFlash(void)
{
   return killflash;
}
