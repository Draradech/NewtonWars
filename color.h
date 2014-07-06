#ifndef _COLOR_H_
#define _COLOR_H_

typedef struct
{
   double r;
   double g;
   double b;
} rgb;

typedef struct
{
  double h;
  double s;
  double v;
} hsv;

hsv rgb2hsv(rgb in);
rgb hsv2rgb(hsv in);

#endif /* _COLOR_H_ */

