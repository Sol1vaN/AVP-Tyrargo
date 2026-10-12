#include "fidget_timer.h"
/* Cosmetic RNG: only the first seed is drawn from the game's random stream. */
static uint32_t FidgetRandom(FIDGET_TIMER *timer)
{
 uint32_t x=timer->randomState;
 if(!x)x=0x6d2b79f5u;
 x^=x<<13; x^=x>>17; x^=x<<5;
 return timer->randomState=x;
}
void FidgetTimerReset(FIDGET_TIMER *timer)
{
 timer->remaining=0;
 timer->armed=0;
}
int FidgetTimerReady(FIDGET_TIMER *timer, int elapsedFixed)
{
 if(elapsedFixed<=0)return 0;
 if(!timer->armed) {
  timer->remaining=FIDGET_WAIT_MIN+(int)(FidgetRandom(timer)%(FIDGET_WAIT_MAX-FIDGET_WAIT_MIN+1u));
  timer->armed=1;
  return 0; /* The preceding frame may belong to an animation or another state. */
 }
 if(elapsedFixed<timer->remaining) {
  timer->remaining-=elapsedFixed;
  return 0;
 }
 FidgetTimerReset(timer);
 return 1; /* Never catch up with a burst of animations after a long frame. */
}
