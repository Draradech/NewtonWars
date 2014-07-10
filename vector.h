#ifndef _VECTOR_H_
#define _VECTOR_H_

typedef struct
{
   double x;
   double y;
} Vec2d;

typedef struct
{
   float x;
   float y;
} Vec2f;

Vec2d vsub(Vec2d v1, Vec2d v2);
Vec2d vadd(Vec2d v1, Vec2d v2);
Vec2d vmul(Vec2d v, double d);
Vec2d vdiv(Vec2d v, double d);
double length(Vec2d v);
double distance(Vec2d v1, Vec2d v2);

Vec2f d2f(Vec2d v);
Vec2d f2d(Vec2f v);

#endif /* _VECTOR_H_ */
