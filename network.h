#ifndef _NETWORK_H_
#define _NETWORK_H_

#define MSG_OWNID 1
#define MSG_PLAYERLEAVE 2
#define MSG_PLAYERPOS 3
#define MSG_SHOTFINISHED 4 /* not sent for bot protocol >= 6 */

#define MSG_SHOTBEGIN 5
#define MSG_SHOTFIN 6 /* replaces msg 4 */
#define MSG_GAMEMODE 7 /* not sent for bot protocol > 8 */
#define MSG_OWN_ENERGY 8
#define MSG_PLANETPOS 9

#define MODE_REALTIME 3

#include "simulation.h"

void initNetwork(void);
void stepNetwork(void);

void sendPlanets(int i);
void allSendPlayerPos(int p);
void allSendKillMessage(int p, int p2);
void allSendShotFinished(SimShot* s);
void allSendShotBegin(SimShot* s);
void disconnectPlayer(int p);

#endif /* _NETWORK_H_ */
