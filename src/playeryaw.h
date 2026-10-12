#ifndef AVP_PLAYER_YAW_H
#define AVP_PLAYER_YAW_H
#include <stdint.h>
/* MATRIXCH is supplied by 3dc.h. This state belongs only to the local player. */
double PlayerYawRadians(const MATRIXCH *orientation);
void PlayerYawReset(void);
void PlayerYawApply(MATRIXCH *orientation, int64_t angleFixed);
void PlayerYawNormalise(MATRIXCH *orientation);
#endif
