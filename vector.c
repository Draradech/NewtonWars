#include <math.h>

#include "vector.h"

Vec2d vsub(Vec2d v1, Vec2d v2)
{
   Vec2d res;
   res.x = v1.x - v2.x;
   res.y = v1.y - v2.y;
   return res;
}

Vec2d vadd(Vec2d v1, Vec2d v2)
{
   Vec2d res;
   res.x = v1.x + v2.x;
   res.y = v1.y + v2.y;
   return res;
}

Vec2d vmul(Vec2d v, double d)
{
   Vec2d res;
   res.x = v.x * d;
   res.y = v.y * d;
   return res;
}

Vec2d vdiv(Vec2d v, double d)
{
   Vec2d res;
   res.x = v.x / d;
   res.y = v.y / d;
   return res;
}

double length(Vec2d v)
{
   return sqrt(v.x*v.x + v.y*v.y);
}

double distance(Vec2d v1, Vec2d v2)
{
   return length(vsub(v1, v2));
}