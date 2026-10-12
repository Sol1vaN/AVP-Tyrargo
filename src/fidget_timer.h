#ifndef AVP_FIDGET_TIMER_H
#define AVP_FIDGET_TIMER_H
#include <stdint.h>
#define FIDGET_WAIT_MIN (5 * 65536)
#define FIDGET_WAIT_MAX (12 * 65536)
typedef struct FIDGET_TIMER { int remaining; uint32_t randomState; int armed; } FIDGET_TIMER;
void FidgetTimerReset(FIDGET_TIMER *timer);
int FidgetTimerReady(FIDGET_TIMER *timer, int elapsedFixed);
#endif
