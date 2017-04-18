#ifndef _DISPLAY_H_
#define _DISPLAY_H_
#include "color.h"

#if defined(__MACH__)
void idleFunc();
#endif

void updateZoom(double z);
void toggleFps(void);
rgb getColor(int player);

void initDisplay(int* argc, char** argv);
void stepDisplay(void);

#endif /* _DISPLAY_H_ */
