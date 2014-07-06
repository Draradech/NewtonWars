#include "color.h"

hsv rgb2hsv(rgb in)
{
   hsv out;
   double min, max, delta;

   min = in.r < in.g ? in.r : in.g;
   min = min  < in.b ? min  : in.b;

   max = in.r > in.g ? in.r : in.g;
   max = max  > in.b ? max  : in.b;

   out.v = max;
   delta = max - min;
   if(max > 0.0)
   {
      out.s = (delta / max);
   }
   else
   {
      out.s = 0.0;
      out.h = 0.0; /* NaN */
      return out;
   }

   if(in.r == max)
   {
      out.h = (in.g - in.b) / delta;
   }
   else if(in.g == max)
   {
      out.h = 2.0 + (in.b - in.r) / delta;
   }
   else
   {
      out.h = 4.0 + (in.r - in.g) / delta;
   }

   out.h *= 60.0;

   if(out.h < 0.0)
   {
      out.h += 360.0;
   }

   return out;
}

rgb hsv2rgb(hsv in)
{
   double p, q, t, ff;
   long i;
   rgb out;

   if(in.s <= 0.0)
   {
      out.r = in.v;
      out.g = in.v;
      out.b = in.v;
      return out;
   }
   if(in.s > 1.0)
   {
      in.s = 1.0;
   }
   while(in.h >= 360.0)
   {
      in.h -= 360.0;
   }
   while(in.h < 0.0)
   {
      in.h += 360.0;
   }
   in.h /= 60.0;
   i = (long)in.h;
   ff = in.h - i;
   p = in.v * (1.0 - in.s);
   q = in.v * (1.0 - (in.s * ff));
   t = in.v * (1.0 - (in.s * (1.0 - ff)));

   switch(i)
   {
      case 0:
      {
         out.r = in.v;
         out.g = t;
         out.b = p;
         break;
      }
      case 1:
      {
         out.r = q;
         out.g = in.v;
         out.b = p;
         break;
      }
      case 2:
      {
         out.r = p;
         out.g = in.v;
         out.b = t;
         break;
      }
      case 3:
      {
         out.r = p;
         out.g = q;
         out.b = in.v;
         break;
      }
      case 4:
      {
         out.r = t;
         out.g = p;
         out.b = in.v;
         break;
      }
      case 5:
      default:
      {
         out.r = in.v;
         out.g = p;
         out.b = q;
         break;
      }
   }
   return out;     
}
