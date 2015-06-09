#ifndef _NETWORK_H_
#define _NETWORK_H_

#define MSG_OWNID 1
#define MSG_PLAYERLEAVE 2
#define MSG_PLAYERPOS 3
#define MSG_SHOTFINISHED 4

#include "simulation.h"

void initNetwork(void);
void stepNetwork(void);

void allSendPlayerPos(int p);
void allSendShotFinished(SimShot* s);

extern int overdrive;

#endif /* _NETWORK_H_ */
